# Native class registration parents

`ffxivgame.exe` registers the engine's Lua classes natively at boot, each with
an explicit parent class name. The parent is a Lua registry relationship that
the class record stores at `+0x0C`. It is independent of MSVC RTTI
inheritance. This page tabulates every such registration, the parent each one
names, and the native object each one constructs.

Labels: STATIC means read from instructions or data in the pinned executable.
INFERRED marks an interpretation that the instructions do not settle by
themselves.

## Inputs

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe`, 1.23b `2012.09.19.0001`, PE32 x86, 15,996,808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base `0x00400000`. Addresses are VAs |
| Tools | Capstone 5.0.7 x86-32 over the original file bytes, driven by [`tools/extract_class_registrations.py`](../../tools/extract_class_registrations.py). Disassembler and decompiler output from other tools was used only as a lead |
| Ranges | `[0x0075F9B0, 0x007616AB]`, the registrar, and `[0x0078FFAF, 0x0078FFF9]` inside `0x0078FC90` |

## Registration calls

STATIC. The registrar at `0x0075F9B0` is called from `0x00763669`, the boot
step recorded in [Lua bootstrap chunks](lua-bootstrap-chunks.md). It makes 40
registrations. Each one has the same shape:

1. A string is built from the parent literal, then a second string from the
   child literal, both through `0x00447260` with the literal address and the
   length `-1` read from `0x00F67298`. The parent is always built first.
2. The strings are passed by address on the stack. For `0x00CC9760` the
   parent string is pushed first and the child string last. For `0x00CC9700`
   the pushes are parent, child, then the object. The last push is argument 1
   in both cases.
3. For `0x00CC9700` the first argument is a pointer to a native object that
   the registrar allocated through `0x009D1B35` and constructed with
   `mov ecx, eax` followed by a call. `0x00CC9760` has no object.

Both entry points are thiscall-style. The instruction directly before each of
the 41 calls is `mov ecx, esi`, and `0x00CC9760` begins with `mov ecx,[ecx]`
before its tail jump. The stack carries the arguments listed below.

The two entry points take these arguments:

| Entry | Stack arguments | Cleanup | Path |
|---|---|---|---|
| `0x00CC9700` | object, name, parent | `ret 0xC` at `0x00CC975D` | Forwards to `0x00CD9F30`, which reaches `0x00CD7970` with the name and passes the parent to `0x00CD9160`, which resolves it through `0x00CD8870` and stores it at child `+0x0C` through `0x00CE1600` |
| `0x00CC9760` | name, parent | `ret 8` at `0x00CDA68A` | Tail-jumps to `0x00CDA610`, which calls `0x00CD91E0` with the same pair. That function raw-reads the name as a global and creates its table when it is not one, and requires the parent name to resolve to an existing record. It stores that parent at child `+0x0C` through `0x00CE1600` at `0x00CD933D`, or passes it to `0x00CD9160` at `0x00CD9309` |

Both entry points order the pair as child then parent. The first argument of
`0x00CC9760` is the child because `0x00CD91E0` uses it as the name it creates.
The second is a name that must already resolve to a record.

## Parent table

STATIC unless the last column says otherwise. The call site is the address of
the call instruction. Native object gives the allocation size and the
constructor called on it. Rows appear in address order. The last row is the
only registration outside the registrar.

| Call site | Entry | Registered name | Registered parent | Native object | Tier |
|---|---|---|---|---|---|
| `0x0075FA83` | `0x00CC9700` | `ActorBaseClass` | (empty string) | `0x60`, `0x006DBB70` | INFERRED |
| `0x0075FB69` | `0x00CC9700` | `CharaBaseClass` | `ActorBaseClass` | `0xE8`, `0x006ECD80` | STATIC |
| `0x0075FC4C` | `0x00CC9700` | `PlayerBaseClass` | `CharaBaseClass` | `0xF8`, `0x006ED720` | STATIC |
| `0x0075FD30` | `0x00CC9700` | `NpcBaseClass` | `CharaBaseClass` | `0x160`, `0x006F3650` | STATIC |
| `0x0075FE10` | `0x00CC9700` | `WidgetBaseClass` | `ActorBaseClass` | `0x68`, `0x006E0080` | STATIC |
| `0x0075FE97` | `0x00CC9760` | `DesktopWidgetBaseClass` | `WidgetBaseClass` | None | STATIC |
| `0x0075FF62` | `0x00CC9700` | `DesktopWidget` | `DesktopWidgetBaseClass` | `0x80`, `0x006E0650` | STATIC |
| `0x00760042` | `0x00CC9700` | `DesktopUtil` | `DesktopWidgetBaseClass` | `0x68`, `0x00776C90` | STATIC |
| `0x00760123` | `0x00CC9700` | `DirectorBaseClass` | `ActorBaseClass` | `0x70`, `0x006F1310` | STATIC |
| `0x007601A9` | `0x00CC9760` | `WorldBaseClass` | `ActorBaseClass` | None | STATIC |
| `0x00760275` | `0x00CC9700` | `AreaBaseClass` | `ActorBaseClass` | `0xBC`, `0x006F3210` | STATIC |
| `0x00760358` | `0x00CC9700` | `PrivateAreaBaseClass` | `AreaBaseClass` | `0x114`, `0x006F3D90` | STATIC |
| `0x007603DF` | `0x00CC9760` | `ZoneBaseClass` | `AreaBaseClass` | None | STATIC |
| `0x0076044D` | `0x00CC9760` | `SystemBaseClass` | `ActorBaseClass` | None | STATIC |
| `0x007604BB` | `0x00CC9760` | `ProgDebugBaseClass` | `SystemBaseClass` | None | STATIC |
| `0x00760584` | `0x00CC9700` | `DebugBaseClass` | `ActorBaseClass` | `0x60`, `0x006DC990` | STATIC |
| `0x00760664` | `0x00CC9700` | `JudgeBaseClass` | `ActorBaseClass` | `0x64`, `0x00776DF0` | STATIC |
| `0x00760745` | `0x00CC9700` | `CommandBaseClass` | `ActorBaseClass` | `0x64`, `0x007588C0` | STATIC |
| `0x00760825` | `0x00CC9700` | `StatusBaseClass` | `ActorBaseClass` | `0x64`, `0x00776200` | STATIC |
| `0x00760906` | `0x00CC9700` | `QuestBaseClass` | `ActorBaseClass` | `0x64`, `0x00776F50` | STATIC |
| `0x007609E6` | `0x00CC9700` | `CommandDebuggerBaseClass` | `ActorBaseClass` | `0x60`, `0x006DC3E0` | STATIC |
| `0x00760A6D` | `0x00CC9760` | `CommandDebuggerGM` | `CommandDebuggerBaseClass` | None | STATIC |
| `0x00760ADB` | `0x00CC9760` | `CommandDebuggerDEV` | `CommandDebuggerBaseClass` | None | STATIC |
| `0x00760B49` | `0x00CC9760` | `CommandDebuggerTEST` | `CommandDebuggerBaseClass` | None | STATIC |
| `0x00760BB8` | `0x00CC9760` | `CommandDebuggerFUNCBaseClass` | `CommandDebuggerBaseClass` | None | STATIC |
| `0x00760C80` | `0x00CC9700` | `ItemBaseClass` | `ActorBaseClass` | `0x74`, `0x006DD450` | STATIC |
| `0x00760D06` | `0x00CC9760` | `ImportantItemBaseClass` | `ItemBaseClass` | None | STATIC |
| `0x00760D75` | `0x00CC9760` | `MoneyItemBaseClass` | `ItemBaseClass` | None | STATIC |
| `0x00760DE3` | `0x00CC9760` | `NormalItemBaseClass` | `ItemBaseClass` | None | STATIC |
| `0x00760E51` | `0x00CC9760` | `GameDataBaseClass` | `ActorBaseClass` | None | STATIC |
| `0x00760F1D` | `0x00CC9700` | `SpreadSheet` | `GameDataBaseClass` | `0xDC`, `0x006F67C0` | STATIC |
| `0x00761000` | `0x00CC9700` | `CutScene` | `GameDataBaseClass` | `0xC4`, `0x006DF3F0` | STATIC |
| `0x007610E1` | `0x00CC9700` | `GroupBaseClass` | `ActorBaseClass` | `0x78`, `0x006DCFF0` | STATIC |
| `0x007611C1` | `0x00CC9700` | `Debug` | `ProgDebugBaseClass` | `0x60`, `0x006DC5D0` | STATIC |
| `0x007612A2` | `0x00CC9700` | `Global` | `SystemBaseClass` | `0x78`, `0x006F15A0` | STATIC |
| `0x00761382` | `0x00CC9700` | `Math` | `SystemBaseClass` | `0x68`, `0x006DDD80` | STATIC |
| `0x00761463` | `0x00CC9700` | `String` | `SystemBaseClass` | `0x64`, `0x007770B0` | STATIC |
| `0x00761543` | `0x00CC9700` | `Table` | `SystemBaseClass` | `0x64`, `0x00777210` | STATIC |
| `0x00761624` | `0x00CC9700` | `WorldMaster` | `WorldBaseClass` | `0x64`, `0x006DF130` | STATIC |
| `0x007616AB` | `0x00CC9760` | `OtherArea` | `WorldBaseClass` | None | STATIC |
| `0x0078FFF4` | `0x00CC9760` | `Player` | `PlayerBaseClass` | None | STATIC |

Counts are 26 calls through `0x00CC9700` and 15 through `0x00CC9760`. A byte
scan of the `.text` section for `call` and `jmp` rel32 instructions finds no
other caller of either entry. All 26 `0x00CC9700` calls and 14 of the
`0x00CC9760` calls are in the registrar. The 15th is `0x0078FFF4`, in
`0x0078FC90`, which registers `Player` with the same two-string shape. The
strings are `PlayerBaseClass` at `0x00FE0898`, built first at `0x0078FFBE`, and
`Player` at `0x00FE08A8`, built second at `0x0078FFDB`.

The empty-string parent of `ActorBaseClass` is the literal at `0x00FD865F`. The
instructions show the registration with an empty parent name. Reading that as
no parent link is INFERRED, because the lookup of an empty name through
`0x00CD8870` was not traced.

## Hierarchy

```
(none)
  ActorBaseClass
    CharaBaseClass
      PlayerBaseClass
        Player
      NpcBaseClass
    WidgetBaseClass
      DesktopWidgetBaseClass
        DesktopWidget
        DesktopUtil
    DirectorBaseClass
    WorldBaseClass
      WorldMaster
      OtherArea
    AreaBaseClass
      PrivateAreaBaseClass
      ZoneBaseClass
    SystemBaseClass
      ProgDebugBaseClass
        Debug
      Global
      Math
      String
      Table
    DebugBaseClass
    JudgeBaseClass
    CommandBaseClass
    StatusBaseClass
    QuestBaseClass
    CommandDebuggerBaseClass
      CommandDebuggerGM
      CommandDebuggerDEV
      CommandDebuggerTEST
      CommandDebuggerFUNCBaseClass
    ItemBaseClass
      ImportantItemBaseClass
      MoneyItemBaseClass
      NormalItemBaseClass
    GameDataBaseClass
      SpreadSheet
      CutScene
    GroupBaseClass
```

`PlayerBaseClass` and `NpcBaseClass` sit under `CharaBaseClass`, not directly
under `ActorBaseClass`. `Debug` and `DebugBaseClass` are different classes with
different parents.

## Cross-checks

- `xivl-client-structs:docs/door-initialization-native-contract.md`, section
  Superclass dispatch, records `NpcBaseClass` under `CharaBaseClass`
  with literals `0x00FDEBB0` and `0x00FDEBC0`, constructor `0x006F3650` and
  call `0x0075FD30`. All of these agree with the table.
- [Widget work members](widget-work-members.md), section Widget class
  registration, lists `WidgetBaseClass`, `DesktopWidgetBaseClass`,
  `DesktopWidget` and `DesktopUtil` with the same parents, sizes and
  constructors. The call sites here fall at the end of each site range there.
- `0x0075D9E0`, called from `0x0076366F`, constructs 24 class-name strings and
  hands each with three dwords to `0x00CC9950` in a loop of 24 records of
  `0x60` bytes. It contains no call to `0x00CC9700` or `0x00CC9760` and no
  parent literal, so it is not a parent table. It lists `ActorBaseClass`,
  `CharaBaseClass`, `PlayerBaseClass`, `NpcBaseClass`, `WorldBaseClass`,
  `AreaBaseClass`, `ZoneBaseClass`, `PrivateAreaBaseClass`,
  `DirectorBaseClass`, `JudgeBaseClass`, `QuestBaseClass`, `CommandBaseClass`,
  `StatusBaseClass`, `GroupBaseClass`, `ItemBaseClass`,
  `ImportantItemBaseClass`, `MoneyItemBaseClass`, `NormalItemBaseClass`,
  `CommandDebuggerBaseClass`, `CommandDebuggerFUNCBaseClass`,
  `DebugBaseClass`, `SystemBaseClass`, `ProgDebugBaseClass` and
  `GameDataBaseClass`. The meaning of the three dwords is not established
  here.
- [Lua class registry](lua-class-registry.md) lists script paths for most of
  these names and points to this page for the `SystemBaseClass` parent.

## Open items

- `ProgDebugBaseClass` has a registration and a parent but no script path
  established here. Its name appears as a literal at `0x00FDED40`,
  `0x00FDF004`, `0x00FDF254` and `0x00FE064C`. The only path literal is
  `/prog/ProgDebugBaseClass`, the chunk name of the 134-byte embedded source
  loaded at `0x0079006A` (see [Lua bootstrap chunks](lua-bootstrap-chunks.md)).
  The [Lua class registry](lua-class-registry.md) row for this class lists
  that same string as its source path, marked debug build only. The decoded
  script registry in `xivl-client-scripts:lua/registry.json` has no entry for
  it. Whether the tree resolves another path for this class is not
  established.
- Whether an empty parent name leaves child `+0x0C` null is INFERRED, as above.
- The three dwords per class built by `0x0075D9E0` are not decoded.
- The table records registration order and parents at boot. It does not show
  whether later script code changes a parent.

## Reproduction

1. Verify the executable size and SHA-256 above.
2. Run `python3 tools/extract_class_registrations.py --exe orig/ffxivgame.exe`
   with Capstone installed. It linearly disassembles the two ranges, tracks
   the stack-slot addresses of the pushed strings and the stack cleanup of
   each callee, and reads the literals from the file. It stops with an error
   when the slot reading and the string construction order disagree. Add
   `--json` to emit the literal addresses and construction data.
3. Check the call-site set with
   `python3 tools/extract_class_registrations.py --exe orig/ffxivgame.exe --callers`.
   It scans the `.text` section for `E8` and `E9` rel32 instructions whose
   target is `0x00CC9700` or `0x00CC9760` and needs no Capstone.
4. Confirm the callee argument roles by disassembling `0x00CC9700`,
   `0x00CD9F30`, `0x00CD9160`, `0x00CE1600`, `0x00CC9760`, `0x00CDA610` and
   `0x00CD91E0` and following the stack-argument offsets listed in
   [Registration calls](#registration-calls).
5. Run `python3 tools/test_class_registrations.py`. Its retail case runs when
   `orig/ffxivgame.exe` and Capstone are present.

## Limitations

- The call-site scan covers direct `E8` and `E9` calls only. Indirect calls to
  the two entries were not searched for.
- The table is derived from linear disassembly of known ranges. Each row is
  cross-checked by two readings of the same instructions and by the existing
  Npc and Widget pages, but no live client run was made.
- Argument roles for `0x00CC9760` come from the push order at 15 call sites and
  from the callee reads, not from a run.
- The native object column states only that the allocation is followed by a
  call with the allocation in `ecx`. It does not name the constructed C++
  class.
