# Widget string call sites

The retail 1.23b `ffxivgame.exe` has image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The VAs below were mapped through the PE section table with `pefile`
2024.8.26 and decoded from the original bytes with Capstone 5.0.7 in x86
32-bit mode. The ASCII strings were read from the same executable.

| Vtable owner and slot | Direct call-site observation |
| --- | --- |
| `ItemSearchPriceViewWidget` slot 4, VA `0x0050D9D0` | VA `0x0050DA67` pushes the pointer `0x00F99D14` to `ListBox_RetainerList`, and VA `0x0050DA70` directly calls `0x00447260`. The intervening instruction prepares a stack-local receiver. |
| `RetainerMenuPhase` slot 4, VA `0x0088A520` | VA `0x0088A581` pushes the pointer `0x01054FC0` to `TextBlock_RetainerName_%d`; VA `0x0088A590` calls `0x009D4F83` with that pointer among its arguments. The following sequence passes a stack-local pointer to `0x00447260` at VA `0x0088A5AB`. |

The slot-to-function links are the pinned RTTI-derived entries in
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl)
for vtable RVAs `0x00B9E71C` and `0x00C55264`. The instructions establish
literal arguments and call edges. They do not establish successful widget
lookup, the result of the formatted name, or any retail UI interaction.
