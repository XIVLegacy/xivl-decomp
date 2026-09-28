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

Direct search of the pinned PE confirms each exact literal occurs once with a
trailing NUL, at the listed file offset and RVA. The string presence does not establish
that a corresponding form asset is present, loaded, or used at runtime.

## Additional code-referenced paths

Two more NUL-terminated paths have direct push references in the pinned PE.
Their instruction sites are recorded in
[widget-string-call-sites.md](../script/widget-string-call-sites.md):

| Literal | Code-referenced RVA and file offset | Occurrence count |
|---|---:|---:|
| `\widget\DirectPurchaseWidget.form` | `0x00C49AD0` | 1 |
| `\system\bootup\BootupMenu.form` | `0x00C55608` | 9 |

Direct byte search of the pinned executable confirms one occurrence of
`\widget\DirectPurchaseWidget.form` and nine occurrences of
`\system\bootup\BootupMenu.form`. The linked code-site page records the
pinned-PE push addresses and bytes. The push references do not establish that
the client loads either form. The executable is pinned by SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
