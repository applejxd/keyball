# Keyball39 emacs 配置図

`keymap.c`、`keyball39.h`、`via.json` から自動生成した配置図。

- [SVG](keymap.svg)
- [Combo SVG](combos.svg)
- [PDF](keymap.pdf)
- [自動生成リファレンス](reference.md)
- [Keymap Drawer入力](keymap.yaml)
- [物理レイアウト](layout.json)
- [手動メタデータ](metadata.yaml)

## 生成

```console
mise run keymap:generate
```

生成物が最新かだけを確認する場合:

```console
mise run keymap:check
```

生成処理は次を自動的に反映する。

- `keymap.c` の全レイヤー
- Tap/Hold、Modifier、ワンショットレイヤー
- `via.json` のキー位置と親指キーの回転
- 右トラックボール構成で存在しないキーの除外
- Comboのキーと出力
- JIS配列の記号
- Key Override
- RGBレイヤー色
- Auto Mouse設定
- `rules.mk` の機能フラグ
- カスタムキーと状態遷移

`reference.md`、SVG、PDF、Keymap Drawer入力、物理レイアウトは生成物なので
直接編集しない。

Windows・JIS配列などの前提、カスタムキーの意味、外部アプリに依存する
ショートカット、状態遷移は `metadata.yaml` で管理する。コードから確定できる
配置や設定と、設計者が記録する意味を分離している。

Python依存はスクリプト内のPEP 723メタデータと
`scripts/generate_keymap_diagram.py.lock` で固定している。

生成器は `--keyboard`、`--keymap`、`--ball`、`--output-dir`、`--metadata` を
指定できる。
例えばKeyball61を一時出力する場合:

```console
uv run --locked scripts\generate_keymap_diagram.py --keyboard keyball61 --keymap emacs --ball right --output-dir build\keymaps\keyball61\emacs
```
