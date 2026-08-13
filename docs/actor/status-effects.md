# Status-related UI RTTI search

This page records a bounded search of the retail client's tracked RTTI catalog
for class names associated with status-effect display.

## Keyword results

The case-insensitive search covered the `class` field of all 5,719 records in
`config/ffxivgame.rtti.json`. It did not inspect unnamed objects, function
behavior, indirect calls, or virtual calls.

| Keyword | Matching class records |
|---|---:|
| `BuffEffect` | 0 |
| `StatusEffect` | 0 |
| `EffectIcon` | 0 |
| `StatusIcon` | 0 |
| `StatusBar` | 0 |
| `BuffBar` | 0 |
| `BuffList` | 0 |
| `EffectList` | 7 |
| `EffectPlate` | 0 |
| `StatusList` | 0 |
| `EffectText` | 0 |
| `CharacterBuff` | 0 |

The seven `EffectList` substring matches are effect-listener classes in the
Cut and Vfx namespaces. None is in `Application::Main::Element::*` or an
`Application::Main::HUD::*` namespace. The catalog therefore does not attest a
status-effect display widget under the searched names. This is a class-name
catalog result, not a claim about the rendering implementation.

## Status-related class records

The tracked vtable-slot catalog confirms these four records and counts:

| Class | Vtable RVA | Slots |
|---|---:|---:|
| `Application::Main::Element::Window::Widget::Status` | `0xb9d424` | 1 |
| `Application::Main::Element::Window::Widget::StatusWidget` | `0xb9f7bc` | 40 |
| `Application::Main::Element::System::TargetInfo` | `0xba415c` | 10 |
| `Application::Main::Element::Chara::NamePlate` | `0xbcf98c` | 24 |

Source: `config/ffxivgame.vtable_slots.jsonl`.

## Cross-references

- `docs/actor/architecture.md` - actor and battle architecture
- `docs/actor/damage-display.md` - the character popup family
