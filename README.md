# Saracroche for Sailfish OS (unofficial port)

Blocks French telemarketing calls on Sailfish OS 5 using the lists of
[Saracroche](https://saracroche.org)
([Android source](https://codeberg.org/cbouvat/saracroche-android)).
This port is not affiliated with the original project.

> **Proof of concept**: this app was written with Claude Opus (Anthropic's AI
> model) and has not yet been tested on a real device. Use at your own risk.

The Android app cannot block calls through Android App Support, so this app
feeds the Saracroche lists to the native Sailfish call filter
(`voicecall-filter-plugin`, dconf `/sailfish/voicecall/filter/*`). Blocked
calls are rejected and shown as blocked in the Phone history.

## Install

1. Settings → Untrusted software → allow.
2. Download the `.rpm` from the GitHub Releases page and tap it.
3. Open the app once: it downloads the lists and enables the daily update.

Before uninstalling, choose "Retirer les numéros Saracroche" in the pull-down
menu, otherwise the numbers stay blocked.

## Build

```sh
sfdk config target=SailfishOS-5.0.0.62-aarch64
sfdk build
```

Unit tests (host Qt 5): `cd tests && qmake && make && ./tst_patterns`

## License

GPLv3. Lists © Saracroche, CC BY-NC-SA 4.0.
