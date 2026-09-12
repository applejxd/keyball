# Keyball39 emacs キーマップの容量・最適化調査

調査日: 2026-09-13

対象:

- `qmk_firmware/keyboards/keyball/keyball39/keymaps/emacs`
- リポジトリ: `8cdfcc1c6fb8b76503d64500b32c25bb0f987068`
- QMK Firmware: `0.22.14`
- QMK CLI コンテナ:
  `ghcr.io/qmk/qmk_cli@sha256:2dc05fc9f32efebd6b05c2b8676ee548358bc7e151e9dbf4dac6b6eed4513b07`
- AVR GCC: `8.3.0`
- MCU / bootloader: ATmega32U4 / Caterina

## 結論

変更前は **28,250 / 28,672 bytes** で、空きは **422 bytes
(1.47%)** しかなかった。QMK のビルド表示では使用率 98% となるが、実値は
98.53% であり、機能追加にはかなり厳しい状態だった。

動作を変えない範囲の最適化を適用した結果、**27,614 / 28,672 bytes**、
空き **1,058 bytes (3.69%)** になった。636 bytes を回収できたが、今後
OLED、RGB、コンボなどに機能を追加するには、まだ余裕が大きいとはいえない。

## 適用した最適化

### `SEND_STRING` を単純な待機に使わない

`CUT_LINE` は `SEND_STRING(SS_DELAY(10))` で 10 ms 待機していた。文字列を
送信していないため、`wait_ms(10)` へ置き換え、不要になった
`sendstring_japanese.h` を削除した。

この変更だけで 532 bytes 削減できた。

### 最高レイヤーの計算を再利用する

スクロールモード判定では `get_highest_layer(state)`、RGB 色の判定では
`biton32(state)` と、同じ最高レイヤーを二度計算していた。前者の結果を RGB
判定でも再利用した。

この変更だけで 104 bytes 削減できた。

### レイヤー色を EEPROM へ保存しない

`rgblight_sethsv()` は QMK 0.22.14 では
`rgblight_sethsv_eeprom_helper(..., true)` を呼び、設定を EEPROM へ保存する。
レイヤー遷移ごとの一時的な色に永続化は不要なので、
`rgblight_sethsv_noeeprom()` へ変更した。

容量は変わらないが、レイヤーを切り替えるたびに EEPROM を更新する動作を
避けられる。

### 不要コードと誤解を招くコメントを整理する

- コメントアウト済み処理から残っていた `mod_state` を削除した。LTO により
  既に除去されていたため、容量は変わらない。
- RGB Light はポインティングデバイスには不要であり、現在はレイヤー色の
  表示に使用していることを明記した。
- `DYNAMIC_KEYMAP_LAYER_COUNT` は VIA 無効時には容量へ影響しないことが分かる
  コメントへ変更した。
- `NO_ACTION_ONESHOT` は `OSL(KL_CX)` に必要であるため、無効化できない。
  実際の設定と逆だったコメントを修正した。

## 機能別の容量

各行は変更前の構成から対象機能だけを変更した独立した計測であり、削減量を
単純に合算することはできない。

| 構成 | 使用量 | 空き | 変更前からの削減 |
|---|---:|---:|---:|
| 変更前 | 28,250 bytes | 422 bytes | - |
| `wait_ms` 使用 | 27,718 bytes | 954 bytes | 532 bytes |
| 最高レイヤー計算の再利用 | 28,146 bytes | 526 bytes | 104 bytes |
| OLED 無効 | 24,276 bytes | 4,396 bytes | 3,974 bytes |
| RGB Light 無効 | 25,626 bytes | 3,046 bytes | 2,624 bytes |
| コンボ無効 | 26,426 bytes | 2,246 bytes | 1,824 bytes |
| Key Override 無効 | 26,520 bytes | 2,152 bytes | 1,730 bytes |
| Auto Mouse 無効 | 27,138 bytes | 1,534 bytes | 1,112 bytes |
| `DYNAMIC_KEYMAP_LAYER_COUNT` 削除 | 28,250 bytes | 422 bytes | 0 bytes |
| 未使用 `mod_state` 削除 | 28,250 bytes | 422 bytes | 0 bytes |

## 追加で容量が必要になった場合

優先順位は次のとおり。

1. OLED が不要なら無効化する。単独で約 4 KB と最も効果が大きい。
2. レイヤー色表示が不要なら RGB Light を無効化する。トラックボールや
   Auto Mouse の動作には影響しない。
3. Auto Mouse を使用していなければ無効化する。
4. コンボと Key Override は削減効果が大きいが、現在のマウスボタン入力と
   Emacs 操作を直接失うため、単純な無効化は推奨しない。

## コード上の注意点

`set_mark_active` 中のカーソル移動は `register_code(KC_LSFT)` と
`unregister_code(KC_LSFT)` を使用している。この方式には次の注意点がある。

- 実際に Shift を押したままカーソルキーを離すと、物理 Shift の状態まで
  解除する可能性がある。
- 複数のカーソルキーを重ねて押すと、先に離したキーが Shift を解除する。

必要なら、弱い Mod (`add_weak_mods` / `del_weak_mods`) と押下数の管理へ
変更する。この修正は信頼性向上になる一方、コード量が増える可能性がある
ため、今回の容量最適化には含めていない。

## 検証方法

QMK 0.22.14 のソースへリポジトリの `keyboards/keyball` をコピーし、次を
実行した。

```console
qmk compile -j 4 -kb keyball/keyball39 -km emacs
```

最適化後のビルドは成功し、QMK のサイズ検査結果は次のとおり。

```text
The firmware size is fine - 27614/28672 (96%, 1058 bytes free)
```

`qmk lint -kb keyball/keyball39` も実行したが、変更していない既存ファイル
`keyboards/keyball/keyball39/via.json` に対して
`The file "keyboards/keyball/keyball39/via.json" should not exist!` と報告され、
lint 全体は失敗した。今回のキーマップ最適化に起因する指摘ではない。
