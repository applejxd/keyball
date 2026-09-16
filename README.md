# Keyball Series

![Keyball61](./keyball61/doc/rev1/images/kb61_001.jpg)

Keyball series is keyboard family which have 100% track ball.

Keyboards in the family are:

* Available
    * Keyball39: split + 39 keys + a track ball
    * Keyball44: split + 44 keys + a track ball
    * Keyball61: split + 61 keys + a track ball
* Unavailable
    * Keyball46 (first one!)
    * One47

## Where to Buy

|Keyboard   |Shirogane Lab / 白銀ラボ                                   |Yushakobo / 遊舎工房                       |
|-----------|-------------------------------------------|-----------------------------------------------------------|
|Keyball39  |<https://shiroganelab.com/products/keyball39> |<https://shop.yushakobo.jp/products/5357>  |
|Keyball44  |<https://shiroganelab.com/products/keyball44> |<https://shop.yushakobo.jp/products/8337>  |
|Keyball61  |<https://shiroganelab.com/products/keyball61> |<https://shop.yushakobo.jp/products/5358>  |

## Build Guide

*   Keyball39:
    [English/英語](/keyball39/doc/rev1/buildguide_en.md),
    [日本語/Japanese](./keyball39/doc/rev1/buildguide_jp.md)
*   Keyball44:
    [English/英語](./keyball44/doc/rev1/buildguide_en.md),
    [日本語/Japanese](./keyball44/doc/rev1/buildguide_jp.md)
*   Keyball61:
    [English/英語](./keyball61/doc/rev1/buildguide_en.md),
    [日本語/Japanese](./keyball61/doc/rev1/buildguide_jp.md)

## Firmware

See [document for firmware source code](./qmk_firmware/keyboards/keyball/readme.md).

### Build with mise

From the repository root, build the **Keyball39 Emacs** firmware with:

```console
mise run keymap:build
```

This task requires mise with uv, a running Docker engine, the pinned QMK CLI
image, and QMK **0.22.14** with its submodules. Place QMK in `build/qmk/`, or
set `QMK_HOME` to an existing checkout. The QMK source and Docker image are
not downloaded automatically.

The task mounts this repository's current Keyball source and performs a clean
build. See the [setup and build instructions](./qmk_firmware/keyboards/keyball/keyball39/keymaps/emacs/README.md#ローカルでビルドする場合)
for the Docker image, `--qmk-home`, and native builds.

| Output | Location |
|---|---|
| Firmware to flash | `build/keyball_keyball39_emacs.hex` |
| Intermediate files | `build/.build/` |
| Batch-build logs | `build/build_log/` |

The entire `build/` directory is ignored by Git. CI also publishes firmware
from `build/`; generated keymap diagrams remain under `docs/`.

To build another keyboard or keymap:

```console
mise run keymap:build -- --keyboard keyball44 --keymap default
```

The [Keyball39 Emacs guide](./qmk_firmware/keyboards/keyball/keyball39/keymaps/emacs/README.md)
documents the key layout, editing commands, and experimental Space+Enter
eight-direction movement layer.

### Pre-compiled Firmwares

(TO BE DOCUMENTED)
