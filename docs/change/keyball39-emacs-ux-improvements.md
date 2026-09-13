# Keyball39 emacs キーマップの UX 改善案

作成日: 2026-09-13

ステータス: 検討中

対象:

- `qmk_firmware/keyboards/keyball/keyball39/keymaps/emacs`
- QMK Firmware 0.22.14
- ATmega32U4 / Caterina

## 目的

現在のキー配置と Emacs 操作の筋肉記憶をできるだけ維持しながら、
トラックボール、選択操作、レイヤー切り替えの予測可能性を高める。

容量だけを減らすのではなく、誤操作の防止、状態の可視化、設定の理解しやすさを
含めて改善する。

## 現状

容量最適化後のファームウェアは **27,614 / 28,672 bytes** で、空きは
**1,058 bytes** である。詳細は
[容量・最適化調査](../research/keyball39-emacs-optimization.md)を参照。

現在のレイヤー構成は次のとおり。

| レイヤー | 用途 |
|---|---|
| `KL_BASE` | 通常入力 |
| `KL_SMB` | 記号、垂直スクロール |
| `KL_NUMFN` | テンキー、ファンクションキー |
| `KL_EMACS` | Emacs操作、水平スクロール |
| `KL_CX` | `C-x` prefix相当のワンショット操作 |

## 優先度1: Auto Mouseの整理

現在は次の設定により、Auto Mouseの対象が `KL_NUMFN` になっている。

```c
#define AUTO_MOUSE_DEFAULT_LAYER 2
```

レイヤー2はマウス専用ではないため、Auto Mouseが有効な状態でトラックボールを
動かすと、テンキー・Fnレイヤーが有効になる。

想定される問題:

- トラックボール操作直後の入力が数字やFキーになる可能性がある
- `J`、`K`、`L` が別のキーコードになるため、クリックコンボと競合する
- `KL_NUMFN` が `KL_SMB` より優先され、垂直スクロールを隠す
- Auto MouseとNumFnが同じRGB色・レイヤー表示になり、状態を区別できない
- `emacs` キーマップ内に `AML_TO` や `KBC_SAVE` がなく、設定を管理しにくい

### 案A: Auto Mouseを無効化する

Auto Mouseを意識して使用していない場合の案。

- 隠れたレイヤー切り替えをなくせる
- 実測で1,112 bytesを回収できる
- 現在のクリックコンボはそのまま使用できる

### 案B: 専用Mouseレイヤーを追加する

Auto Mouseを使用する場合の推奨案。

```c
enum keymap_layer {
    KL_BASE,
    KL_SMB,
    KL_NUMFN,
    KL_EMACS,
    KL_CX,
    KL_MOUSE,
};

#define AUTO_MOUSE_DEFAULT_LAYER KL_MOUSE
```

`KL_MOUSE` には次の機能を直接配置する。

- `KC_BTN1`～`KC_BTN5`
- 押している間だけスクロールする `SCRL_MO`
- 必要なら手動アクセス用の `MO(KL_MOUSE)` または `TG(KL_MOUSE)`

クリックを単一キーへ移した場合、現在のCombo機能を外せる可能性がある。
Combo無効化では単独で1,824 bytes削減できたが、Mouseレイヤー追加後の正味容量は
別途ビルドして計測する。

## 優先度2: Mark操作の安定化

現在はマーク中のカーソル移動で物理Shiftを直接登録・解除している。

```c
register_code(KC_LSFT);
unregister_code(KC_LSFT);
```

この方式には次の問題がある。

- 物理Shiftを押したままカーソルキーを離すと、物理Shiftまで解除する可能性がある
- 複数のカーソルキーを重ねると、先に離したキーがShiftを解除する
- 内部のマーク状態と実際のShift状態がずれても確認しにくい

弱いModifierを使用し、押下中のナビゲーションキー数を管理する方式へ変更する。

- `add_weak_mods(MOD_BIT(KC_LSFT))`
- `del_weak_mods(MOD_BIT(KC_LSFT))`
- 押下数が0から1になったときだけ追加
- 押下数が1から0になったときだけ解除
- `ABORT` やマーク終了時にも状態を整合させる

キー配置や操作方法は変更しない。

## 優先度3: 状態の可視化

既存のOLEDとRGBを利用し、次の状態を簡潔に表示する。

| 状態 | 表示案 |
|---|---|
| Mark有効 | OLEDに `M` または `MARK` |
| `C-x` 入力待ち | OLEDに `CX`、または専用RGB色 |
| Auto Mouse有効 | 既存表示を維持 |
| Mouseレイヤー有効 | 専用RGB色 |
| スクロール中 | OLEDに `S`、またはスクロール方向を表示 |

AVRの容量を圧迫しないよう、固定文字列を使用し、`sprintf` やアニメーションは
追加しない。

## 優先度4: `C-x` ワンショットの予測可能性

`OSL(KL_CX)` は、Emacsの「`C-x` の次に1キー」という構造と対応しているため、
基本設計は維持する。

改善候補:

- `ONESHOT_TIMEOUT` を設定し、放置された入力待ちを自動解除する
- OLEDまたはRGBで入力待ち状態を表示する
- `ABORT` でワンショット状態も確実に解除する

## 優先度5: Comboの誤発火対策

現在のマウスボタンCombo:

| Combo | 操作 |
|---|---|
| `J + K` | 左クリック |
| `K + L` | 右クリック |
| `J + L` | 中クリック |
| `M + ,` | 戻る |
| `, + .` | 進む |

通常入力中のロールで意図せずクリックする場合は、QMK 0.22.14で利用可能な
次の設定を検討する。

- `COMBO_TERM`
- `COMBO_STRICT_TIMER`
- `COMBO_ONLY_FROM_LAYER`
- `combo_should_trigger()`

`COMBO_MUST_TAP` はクリックドラッグを妨げるため、マウスボタンには使用しない。

専用Mouseレイヤーへ移行する場合は、複雑な発火条件を追加するよりCombo機能を
外す方を優先する。

## 優先度6: Layer Tapのキー別調整

次のキーはTapとHoldで役割が異なる。

- `P` / Emacsレイヤー
- `;` / 記号レイヤー
- Space / 記号レイヤー
- `LNG1` / Emacsレイヤー
- Enter / NumFnレイヤー
- `Z`・`/` / Shift

すべてへ同じ判定を適用せず、症状があるキーだけ次のper-key設定で調整する。

- `TAPPING_TERM_PER_KEY`
- `PERMISSIVE_HOLD_PER_KEY`
- `HOLD_ON_OTHER_KEY_PRESS_PER_KEY`

親指のLayer Tapと、通常入力位置の `P`・`;` は別の設定として扱う。

`TAP_CODE_DELAY` はマクロの `tap_code()` に対する待機であり、Layer Tapの
判定時間とは別である。

## スクロール判定の変更

現在は最高レイヤー番号でスクロール方向を決めている。Mouseレイヤーなど、
番号が大きいレイヤーを追加すると既存のスクロール状態を隠す可能性がある。

対象レイヤーの有効状態を直接調べる。

```c
if (layer_state_cmp(state, KL_EMACS)) {
    // horizontal
} else if (layer_state_cmp(state, KL_SMB)) {
    // vertical
} else {
    // disabled
}
```

Mouseレイヤーでは `SCRL_MO` により明示的にスクロールする案も比較する。

## 将来候補: Repeat / Alternate Repeat

QMK 0.22.14ではRepeat KeyとAlternate Repeatを使用できる。Emacsでは次のような
対になる操作に利用できる。

- 左 / 右
- 上 / 下
- Home / End
- Page Up / Page Down
- `C-f` / `C-b`
- `C-n` / `C-p`

現在の空き容量は大きくないため、Auto MouseやComboを整理してから追加し、
ビルドごとに容量を計測する。

Layer Lock、Chordal Hold、Flow Tapなど、QMK 0.22.14に存在しない機能だけを目的に
QMKを更新することは推奨しない。更新時はKeyball独自のsplit通信、
pointing device、OLED、RGBを広範囲に再検証する必要がある。

## READMEの改善

配置図と仕様リファレンスの自動生成を導入した。

コードや設定から機械的に確定できる次の項目は
`docs/spec/keymaps/keyball39/emacs/reference.md` へ自動出力する。

- レイヤー一覧と用途
- 各キーのTapとHold
- 全レイヤーの配置図
- Combo一覧
- Key Override一覧
- RGB色とレイヤーの対応
- Auto Mouse設定
- `rules.mk` の機能フラグ
- カスタムキーコード一覧

コードから意図を安全に推測できない次の項目は
`docs/spec/keymaps/keyball39/emacs/metadata.yaml` で手動管理し、自動生成
リファレンスへ合成する。

- Windows・JIS配列を前提としていること
- `KC_LNG1`、`KC_LNG2`、`KC_INT1`、`KC_INT3` の用途
- `G(S(KC_F23))` など外部アプリ依存ショートカットの用途
- Mark、ABORT、`C-x` の状態遷移と設計意図

READMEは生成コマンドと、自動生成・手動管理の境界だけを説明する。

## 実施順序

一度にすべて変更せず、次の順で個別にビルド・動作確認する。

1. Auto Mouseが実際に使用されているか確認する
2. 未使用なら無効化、使用中なら専用 `KL_MOUSE` を試作する
3. Mark処理をweak modifier化する
4. OLEDへMarkと `C-x` の状態を表示する
5. `ONESHOT_TIMEOUT` とABORT処理を追加する
6. 実際に誤判定するLayer Tapだけ調整する
7. 容量に余裕があればRepeat Keyを試す

各段階で次を確認する。

- ファームウェアが28,672 bytes以内であること
- 通常入力、Combo、クリックドラッグが従来どおり動作すること
- Mark中の選択とABORTが安定すること
- レイヤーを重ねてもスクロール方向が意図どおりであること
- OLEDとRGBの表示が実際の状態と一致すること

## 参考資料

- [QMK 0.22.14 Auto Mouse](https://github.com/qmk/qmk_firmware/blob/0.22.14/docs/feature_pointing_device.md#automatic-mouse-layer)
- [QMK 0.22.14 Combo](https://github.com/qmk/qmk_firmware/blob/0.22.14/docs/feature_combo.md)
- [QMK 0.22.14 Tap-Hold](https://github.com/qmk/qmk_firmware/blob/0.22.14/docs/tap_hold.md)
- [QMK 0.22.14 Repeat Key](https://github.com/qmk/qmk_firmware/blob/0.22.14/docs/feature_repeat_key.md)
