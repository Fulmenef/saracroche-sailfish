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
docker run --rm -v "$PWD":/work:ro -v "$PWD/RPMS":/out \
  coderus/sailfishos-platform-sdk-aarch64:5.1.0.11 \
  bash -lc 'cp -r /work ~/src && cd ~/src && rm -rf .git && mb2 -t SailfishOS-5.1.0.11-aarch64 build && cp RPMS/*.rpm /out/'
```

To publish a release, bump `Version`/`Release` in `rpm/saracroche-sailfish.spec`,
commit, then push a tag: `git tag v0.1.0-2 && git push origin v0.1.0-2`.
The GitHub Action checks that the tag matches the spec (`v<Version>-<Release>`),
runs the unit tests, builds the RPM and attaches it to the release.

Unit tests (host Qt 5): `cd tests && qmake && make && ./tst_patterns`

## License

GPLv3. Lists © Saracroche, CC BY-NC-SA 4.0.
