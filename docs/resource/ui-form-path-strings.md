# UI form path strings

The pinned `ffxivgame.exe` contains these seven NUL-terminated ASCII form-path
literals in `.rdata`:

| Literal | RVA and file offset |
|---|---:|
| `\widget_c\BazaarEditWidget.form` | `0x00B93BE0` |
| `\widget\ItemSearchWidget.form` | `0x00B977B0` |
| `\widget\ItemSearchListWidget.form` | `0x00B99CD8` |
| `\widget_c\RetainerListWidget.form` | `0x00B9BCDC` |
| `\widget\ItemSearchHelpWidget.form` | `0x00C47550` |
| `\system\bootup\Retainer\RetainerMenu.form` | `0x00C54F94` |
| `\system\bootup\retainer\RetainerNameChangeInput.form` | `0x00C55028` |

The offsets also match the focused UI-asset scan's reported offsets. Direct
search of the pinned PE confirms each exact literal occurs once with a trailing
NUL, at the listed file offset and RVA. The string presence does not establish
that a corresponding form asset is present, loaded, or used at runtime.

## Additional code-referenced paths

Two more NUL-terminated paths have direct push references in the pinned PE.
Their instruction sites are recorded in
[widget-string-call-sites.md](../script/widget-string-call-sites.md):

| Literal | Code-referenced RVA and file offset | Exact occurrences |
|---|---:|---:|
| `\widget\DirectPurchaseWidget.form` | `0x00C49AD0` | 1 |
| `\system\bootup\BootupMenu.form` | `0x00C55608` | 9 |

The direct-purchase reference appears in FF14-Memory
`tools/outputs/lpb/native_helper_expand_20260617/helper_string_refs.csv:700`
(SHA-256 `4b435c7cd64aa70cb6f05ada447da6a9e4fad9d6e1c0f37229854c1ae03d8947`).
The BootupMenu reference is in
`tools/outputs/lpb/native_helper_expand_20260617/helper_notes/helper_88ADD0.md:17`
(SHA-256 `58c8d5e05122669fadc0400da51dbef63527ad159afb99bc2c4d3635121befd3`).
That note's helper attribution is not established by the cited window. The
linked code-site page records the pinned-PE push address and bytes. The
BootupMenu literal has eight other occurrences. The push references do not
establish that the client loads either form.

Sources: FF14-Memory
`tools/outputs/lpb/native_boundary_scan_20260617/native_focus_ui_asset_hits.csv:2-8`
(SHA-256 `dda3926a5ff9e9232495baf393b5c8393c615b73f30648be524c91d51e433016`),
`native_strings_search_hits.csv:2,8,57,67,87,149,151`
(SHA-256 `0e8617293d6edf47a78cfd775738be7c4a33ff312447926c78b6eb8938391ddb`),
and `binary_inventory.csv:2` (SHA-256
`0270acf9638d9d008c8acc12033ed4b446a60b42bbc901cd5d420143e7c96001`). The
executable is pinned by SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
