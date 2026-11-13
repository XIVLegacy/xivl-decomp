# Sub-target guide initialization

The native SubTargetGuideWidget factory entry opens the installed form and
initializes the guide and range-toggle labels. The request that opens this
widget during targeting remains unknown.

## Input and method

The input is retail 1.23b build 2012.09.19.0001. The PE32 x86 executable
`ffxivgame.exe` is 15996808 bytes with image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are VAs. Ghidra 12.1.4 via ghidra-cli 0.2.2,
LLVM disassembly and PE section reads establish the static observations.

## Factory and window initialization

| Locator | Observation |
| --- | --- |
| `0x0066DD26..0x0066DD36`, tables `0x0066E8F0` and `0x0066E7F8` | Factory kind 7 selects jump index 7, which points to `0x0066DE78`. |
| `0x0066DE78..0x0066DEA0` | The selected arm requests 0x238 bytes and calls `0x0052BBD0` at `0x0066DE9B` on success. |
| `0x0052BC00`, `0x0052BC30` | The constructor passes the `widget/SubTargetGuideWidget.form` path at `0x00F9CB5C` to the base path and installs the SubTargetGuideWidget RTTI vtable at `0x00F9F864`. |
| `0x00F9F874`, `0x005119D0` | Slot 4 of that table points to the initialization function. |
| `0x00511A18` | The initializer requests Window IsModal with `0x009243D0(1,0,1)`. The setter uses descriptor `0x0135722C` at `0x009243DB`. |
| `0x00511A2A..0x00511A6A` | It looks up `ToggleButton_RangeAttack` and stores the result at widget +0x234. |
| `0x00511A86..0x00511AC3` | It looks up `TextBlock_GuideText` and stores the result at widget +0x230. |
| `0x00511AD9..0x00511B31` | It formats the `@%d` literal at `0x00F9D274` with 1025 and requests Text on the TextBlock through `0x009556A0`. |
| `0x00511B46..0x00511B9E` | It formats the same literal with 1026 and requests Content on the toggle through `0x009350A0`, whose descriptor is `0x01358F94`. |
| `0x00511BB3..0x00511BBF` | It requests `0x0095D440(0,0,1)` on the toggle. The descriptor identifies this as an IsChecked false request. |

The general [factory and slot route](https://github.com/XIVLegacy/xivl-client-structs/blob/79b7404cf5852f7c35fd26ae2b34762089357f7a/docs/progfunc-native-call-contract.md#callback)
passes the kind to `0x0066DD00` at `0x0066ECBD` and stores the resulting
pointer in the manager at `0x0066ECCE`, indexed by kind. The
[IsModal descriptor](sqwt-window-trigger-order.md#itemselectwidget-modal-show-and-cancel),
[Text setter](damage-plate-form-rules.md#text-role) and
[IsChecked setter](../input/game-controller-startup.md#checked-state-callback-changes-the-image-part-index)
are identified independently. The [Content descriptor](sqwt-plain-text-reference-lookup.md#named-text-properties)
is named by the initializer at `0x00F221E0`.

The native requests are conditional on a successful factory and form load.
The Lua SubTargetGuideWidget class has a separate init body. The installed
`widget/subtargetguidewidget` LPB is identified by the
[retail Lua coverage manifest](https://github.com/XIVLegacy/xivl-client-scripts/blob/f7297bd1d0acf3fcee81e967bdfee8d1c38cac89/manifests/retail_lua_coverage.json#L27224).
Decoding it with the manifest tool and listing the Lua 5.1 instructions
with unluac gives `main/f0` PCs 0 through 7, which request text row 1025
and hide the range toggle.
The native IsChecked request supplies no Visibility value. Which path
opens the guide for the active sub-target, when it closes and which
requests reach the screen remain unknown.
