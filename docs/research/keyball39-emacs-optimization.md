# Keyball39 emacs キーマップの容量・最適化調査

調査日: 2026-09-13
更新日: 2026-09-14

対象:

- `qmk_firmware/keyboards/keyball/keyball39/keymaps/emacs`
- 初回調査のリポジトリ: `8cdfcc1c6fb8b76503d64500b32c25bb0f987068`
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

その後、[UX改善案の優先度1・案A](../change/keyball39-emacs-ux-improvements.md)として
Auto Mouseを廃止した。最適化済み構成との比較ビルドでは
**26,502 / 28,672 bytes**、空き **2,170 bytes (7.57%)** となり、
さらに **1,112 bytes** を回収できた。これは自動レイヤー切り替えを廃止する
機能変更であり、上記の動作を変えない最適化とは区別する。

さらにMarkのShift管理を安定化した結果、**26,752 / 28,672 bytes**、
空き **1,920 bytes (6.70%)** となった。安定化による増加はFlash **250 bytes**、
静的RAM **9 bytes** である。

その後、優先度5のCombo対策として受付時間を30 msへ短縮し、strict timerを
有効化した。現在は **26,764 / 28,672 bytes**、空き **1,908 bytes (6.65%)**。
Combo対策による増加はFlash **12 bytes**、静的RAMは不変である。

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

## Auto Mouse廃止後の再計測

比較元は `b118a3261e573162d55741fb101c339aebe68c34` の最適化済み構成。
QMK 0.22.14 (`ca4541699915b37cd1f253bbed51854627efd2ce`) と上記の
固定コンテナ・AVR GCCを使用し、各ビルド前に `qmk clean` を実行した。
2構成のコード差分は `emacs/config.h` の
`POINTING_DEVICE_AUTO_MOUSE_ENABLE` と `AUTO_MOUSE_DEFAULT_LAYER` の削除だけである。

| 構成 | Flash使用量 | Flash空き | 静的RAM使用量 |
|---|---:|---:|---:|
| 最適化済み・Auto Mouseあり | 27,614 bytes | 1,058 bytes | 1,518 bytes |
| 最適化済み・Auto Mouse廃止 | 26,502 bytes | 2,170 bytes | 1,506 bytes |

Flashは `.text + .data`、静的RAMは `.data + .bss` で計測した。
Intel HEXのチェックサム・実データ量も検証し、ELFのFlash使用量と一致した。
初回の独立計測と削減量は同じだったが、加算による推測ではなく再ビルドによる実測値である。

実効マクロの差分はAuto Mouse関連7定義の削除のみで、
Pointing Device、Combo、Key Override、OLED、RGB、split、One-shot、Tappingは
維持されている。リンク済みELFからAuto Mouse関連シンボルが消え、
マウス送信・5種類のクリックCombo・Markのシンボルが残っていることも確認した。

右トラックボール1個の構成を前提とし、EEPROMの初期化や共有処理の変更は行っていない。
保存形式に関する注意点と実機確認項目は
[UX改善案](../change/keyball39-emacs-ux-improvements.md#eeprom互換性)に記載した。
この比較ビルド時点では実機への書き込みと操作確認は未実施。
その後のCombo対策を含む実機確認結果は、下記の再計測節を参照。

## Mark安定化後の再計測

比較元はAuto Mouse廃止済みの `16d9ea881f3844ffa866702e5e4057a649be6d3f`。
同じQMK 0.22.14・固定コンテナ・AVR GCC 8.3.0を使用し、
各firmwareビルド前に `qmk clean` を実行した。
firmwareソースの差分は `keymap.c` のMark関連処理のみである。

| 構成 | Flash使用量 | Flash空き | 静的RAM使用量 |
|---|---:|---:|---:|
| Auto Mouse廃止直後 | 26,502 bytes | 2,170 bytes | 1,506 bytes |
| Mark安定化後 | 26,752 bytes | 1,920 bytes | 1,515 bytes |

Mark安定化後は `.text=26,458`、`.data=294`、`.bss=1,221` bytes。
HEXチェックサム・実データ量とELFのFlash使用量の一致も確認した。
実効マクロとコンパイルコマンドは比較元と同じで、
Auto Mouseは引き続き無効、既存のPointing Device・Combo・Key Override・
One-shot・Tap-Hold・レイヤー配列は維持されている。

変更前の物理Shift解除・ナビゲーション重ね押し・ABORT保持の3回帰ケースが失敗することを
確認した後、QMK入力処理を使う27件をshuffle seed 914・915・916で実行し、
計81ケースが成功した。テストはproductionの `keymap.c` を直接コンパイルする。
ハードウェア周辺はスタブであり、この時点では実機の選択動作は未検証。
その後の基本動作確認は下記に記録したが、実行時スタック使用量は未測定。

```text
The firmware size is fine - 26752/28672 (93%, 1920 bytes free)
```

`qmk lint -kb keyball/keyball39` は、従来と同じ `via.json` 指摘だけで終了コード1。
回帰テストの再現には `scripts/run_keymap_tests.py --qmk-home <QMKのcheckout先>` を使う。
専用テストとfirmwareビルドを同じQMK checkoutで動かす場合、
`qmk clean` とテストを並行実行しない。

## Combo対策後の再計測

Mark安定化済み構成と、`emacs/config.h` に `COMBO_TERM 30`、
`COMBO_STRICT_TIMER` を追加した構成を比較した。
QMK 0.22.14・固定コンテナ・AVR GCC 8.3.0を使用した。
比較元は新規checkoutでビルドし、変更後は `qmk clean` 後にビルドした。

| 構成 | Flash使用量 | Flash空き | 静的RAM使用量 |
|---|---:|---:|---:|
| Mark安定化後 | 26,752 bytes | 1,920 bytes | 1,515 bytes |
| Combo対策後 | 26,764 bytes | 1,908 bytes | 1,515 bytes |

変更後は `.text=26,470`、`.data=294`、`.bss=1,221` bytes。
Flashは `.text + .data`、静的RAMは `.data + .bss` で計測した。
キー配列とCombo定義は変更していない。

Windowsのbind mount上でGitの作業ツリー走査に時間がかかったため、
両ビルドにQMK標準の `SKIP_GIT=yes` を指定し、バージョンヘッダー生成時の
Git走査だけを省略した。バージョンヘッダーのGit情報は `NA` となる。
QMKのソースバージョンはcheckoutで固定している。

```console
qmk compile -j 4 -kb keyball/keyball39 -km emacs -e SKIP_GIT=yes
```

```text
baseline:
The firmware size is fine - 26752/28672 (93%, 1920 bytes free)
combo:
The firmware size is fine - 26764/28672 (93%, 1908 bytes free)
```

QMK回帰テストは75件（既存Mark 27件＋Combo 48件）が成功し、
shuffle seed 914・915・916でも計225ケースが成功した。
変更前にはCombo境界・タイマー関連8件の失敗を確認した。
`qmk lint -kb keyball/keyball39` は両構成とも既存の `via.json` 指摘だけで失敗し、
出力は一致した。`mise run keymap:check` は成功した。
2026-09-14、QMK Toolboxでの書き込み・確認手順の案内後、
ユーザーから基本動作に問題なしとの報告を受けた。
個別の操作ログや誤発火率の数値、長期使用の結果は記録していない。
詳細は[UX改善案の実機確認結果](../change/keyball39-emacs-ux-improvements.md#実機確認結果)を参照。

## 追加で容量が必要になった場合

優先順位は次のとおり。

1. OLED が不要なら無効化する。単独で約 4 KB と最も効果が大きい。
2. レイヤー色表示が不要なら RGB Light を無効化する。トラックボールや
   Auto Mouse の動作には影響しない。
3. Auto Mouse は廃止済みのため、これ以上の削減対象にはしない。
4. コンボと Key Override は削減効果が大きいが、現在のマウスボタン入力と
   Emacs 操作を直接失うため、単純な無効化は推奨しない。

## Mark実装上の注意点

当初の容量最適化では、物理Shiftを直接登録・解除するMark処理を変更しなかった。
現在は別の安定化変更で、押下位置の追跡とMark専用のweak右Shiftへ移行している。
既存のShift付き記号・マクロはweak左Shiftを使用するため、
両者の解除が干渉しない。物理左右Shiftは従来どおり維持する。

将来weak右Shiftを使うキーやマクロを追加する際は所有の競合を再検討する。
左右Shiftを区別するアプリやリマップ設定での差も、実機確認の対象となる。
詳細は[UX改善案](../change/keyball39-emacs-ux-improvements.md)を参照。

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

Auto Mouse廃止後の比較ビルドも両構成で成功した。

```text
baseline:
The firmware size is fine - 27614/28672 (96%, 1058 bytes free)

without-auto-mouse:
The firmware size is fine - 26502/28672 (92%, 2170 bytes free)
```

両構成の `qmk lint -kb keyball/keyball39` は同じ既存の `via.json` 指摘で
終了コード1となり、新たなlint指摘はなかった。
生成リファレンスは `mise run keymap:generate` で更新し、
`mise run keymap:check` で整合性を確認した。
