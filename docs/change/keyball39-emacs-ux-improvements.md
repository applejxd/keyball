# Keyball39 emacs キーマップの UX 改善案

作成日: 2026-09-13
更新日: 2026-09-14

ステータス: 優先度1の案Aと優先度2を実装済み（実機確認待ち）。優先度3以降は検討中

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

Mark安定化後のファームウェアは **26,752 / 28,672 bytes** で、
空きは **1,920 bytes**。Auto Mouse廃止直後の **26,502 bytes** から
安定化のために **250 bytes** 増加した。比較ビルドの詳細は
[容量・最適化調査](../research/keyball39-emacs-optimization.md)を参照。

現在のレイヤー構成は次のとおり。

| レイヤー | 用途 |
|---|---|
| `KL_BASE` | 通常入力 |
| `KL_SMB` | 記号、垂直スクロール |
| `KL_NUMFN` | テンキー、ファンクションキー |
| `KL_EMACS` | Emacs操作、水平スクロール |
| `KL_CX` | `C-x` prefix相当のワンショット操作 |

## 優先度1: Auto Mouseの整理（案Aを採用）

廃止前は次の設定により、Auto Mouseの対象が `KL_NUMFN` になっていた。

```c
#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define AUTO_MOUSE_DEFAULT_LAYER 2
```

レイヤー2はマウス専用ではないため、Auto Mouseが有効な状態でトラックボールを
動かすと、テンキー・Fnレイヤーが有効になる。

廃止前の設定で想定される問題:

- トラックボール操作直後の入力が数字やFキーになる可能性がある
- `J`、`K`、`L` が別のキーコードになるため、クリックコンボと競合する
- `KL_NUMFN` が `KL_SMB` より優先され、垂直スクロールを隠す
- 自動切替と手動NumFnが同じRGB色・レイヤー番号になり、切替の由来を区別できない
- `emacs` キーマップ内に `AML_TO` や `KBC_SAVE` がなく、設定を管理しにくい

### 案A: Auto Mouseを無効化する（採用）

右トラックボール1個の構成で、自動NumFnを使用しない方針として採用した。
`emacs/config.h` から上記2つの定義を削除し、Auto Mouseをコンパイル対象から
外した。他のキーマップ、共有処理、レイヤー配置、Comboは変更していない。

- ボール操作による自動レイヤー切り替えをなくす
- カーソル移動、5種類のクリックコンボ、ドラッグは維持する
- Enter長押しのNumFn、記号レイヤーの縦スクロール、
  Emacsレイヤーの横スクロールは維持する
- OLEDのAuto Mouse欄は固定のOFF表示と `---` になる
- 再びAuto Mouseを使用するにはファームウェアの変更・再ビルドが必要になる

以前の1,112 bytes削減は、容量最適化前の構成での独立計測値。
今回も最適化済み構成との比較ビルドで同じ1,112 bytesの削減を確認した。

#### EEPROM互換性

Auto Mouseの定義を外すと、共有設定 `keyball_config_t` の `ssnap`
（スクロール方向）がbit 16–17からbit 10–11へ移動する。
既存EEPROMのAuto Mouse設定を方向として読み違える場合があるが、
CPIとスクロール倍率の配置は変わらない。

今回の右トラックボール1個の構成では、通常時はカーソル移動を行い、
記号／Emacsレイヤーへの切り替え時にスクロール方向を設定し直す。
そのため通常のスクロール操作は保存値に依存せず、EEPROMの初期化や
共有設定形式の変更は行わない。起動直後のOLEDの方向表示は異なる場合がある。
現在のemacs配列には設定保存キーがなく、誤読した値を保存し直す処理もない。

2ボール構成では起動直後から従側ボールのスクロール方向に影響し得る。
また、将来 `KBC_SAVE` を追加して保存すると、旧ファームウェアへ戻した際に
設定を読み違える可能性がある。これらの構成へ変更する際は保存形式の
互換性を再検討する。保存・消去を挟まず旧版へ戻せば、元のAuto Mouse設定も
残っている。

#### 実機確認

次は未確認であり、両側へ同じファームウェアを書き込んで確認する。

- ボール操作でNumFnへ切り替わらず、Enter長押しでは切り替わること
- 全5種類のクリック、連続クリック、修飾キー併用、ドラッグとボタン解放
- 縦／横スクロールと、複数レイヤーを重ねた後の切り替え
- ボール操作直後の文字入力で、既存Comboの誤発火が増えないこと
- `C-x` 待機中の中クリック・戻る・進むと、ワンショットの消費・ボタン解放

### 案B: 専用Mouseレイヤーを追加する（見送り）

Auto Mouseを使用する場合の代替案。以下は概念例であり、実装する場合は
`KL_MOUSE` をQMK側からも参照できる共有定義にする必要がある。

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

## 優先度2: Mark操作の安定化（実装済み）

変更前はマーク中のカーソル移動で物理左Shiftを直接登録・解除していた。

```c
register_code(KC_LSFT);
unregister_code(KC_LSFT);
```

この方式には次の問題があった。

- 物理左Shiftを押したままカーソルキーを離すと、物理左Shiftまで解除する
- 複数のカーソルキーを重ねると、先に離したキーがShiftを解除する
- カーソルキーを保持したままABORT等でMarkを終了すると、
  後のキー解放がMark中の処理を通らずShiftが残る

### 維持する操作

`SET_MARK` はキーボード内部の選択モードを切り替え、Emacs固有のコマンドは送らない。
モード中に矢印、Home、End、Page Up、Page Downを押すと補助Shiftを加えるため、
通常のShift選択に対応するエディタや入力欄でも範囲選択できる。

移動キーを離すと補助Shiftを解除するが、選択モードは維持する。
連打でも選択を続けられ、複数の対象キーを重ねた場合は最後のキーを離すまで
補助Shiftを維持する。Mark開始前から保持していたキーは、新規の選択操作としては数えない。

終了条件は従来どおり `SET_MARK` の再押下、Mark中の `ABORT`、
`KC_C`、`C(KC_C)`、`C(KC_X)`、`C(KC_V)`、`KC_DEL` の押下。
Mark中の `ABORT` はEscを送らず、選択モードだけを終了する。
画面上の選択範囲を直接消す操作ではない。
`CUT_LINE` はShift+End、10 ms待機、Ctrl+Xという順序と、Markを終了しない仕様を維持する。

### 実装

- 対象ナビゲーションキーの押下位置を行ごとのビット集合で追跡し、押下数も管理する。
  レイヤー変更後やワンショットの合成解放、古いMarkセッションからの解放でも二重に減算しない。
- Markには `add_weak_mods(MOD_BIT(KC_RSFT))` によるweak右Shiftを使う。
  物理左右Shiftには触れず、既存の `S(kc)` やマクロのweak左Shiftと所有を分離する。
- QMK 0.22.14はキー押下時とComboのバッファ処理後にweak modifierを消すため、
  `pre_process_record_user`、`process_record_user`、`housekeeping_task_user` で
  必要な補助Shiftを復元する。押下数0→1で一度だけ追加する方式にはしない。
- 終了処理を `end_mark()` にまとめ、押下状態とMarkの補助Shiftを解除し、
  必要なキーボードレポートを送る。コピー等の操作にMark由来のShiftを残さない。
- `CUT_LINE` 実行中だけMarkの補助Shiftを外し、Ctrl+Xへの混入を防いでから復元する。
  物理Shiftやマウスボタンの保持状態は変更しない。

キー配置、Combo、Key Override、スクロール、ワンショットの定義は変更していない。
`Alt+B/F/V` のKey Override出力や `Win+Down` 等のModifier付き移動を、
新たにMark対象へ広げることもしていない。

補助ShiftもPCには通常のShiftとして届くため、移動キーを保持しながら文字や
クリックを操作すると、そちらにもShiftが作用し得る。
左右Shiftを区別するアプリやリマップ設定では、補助が左から右へ変わる差を確認する。
今後 `RSFT(kc)` 等のweak右Shiftを使うキーやマクロを追加する場合は、所有の競合を再検討する。

### 検証

`tests/keyball39_emacs_mark` はproductionの `keymap.c` 全体を直接コンパイルし、
QMK 0.22.14の `action_exec`、Combo、Tap-Hold、Key Override、ワンショット、
マクロから送られる実レポートを検証する。
変更前の物理Shift・重ね押し・ABORT保持の3ケースはすべて失敗し、
修正後は同じケースを含む27件が成功した。
shuffle seed 914・915・916の3回、計81ケースが成功し、
`CUT_LINE` の10 ms間隔も確認した。
全production配列を走査し、既存のキーがMark用weak右Shiftと競合しないことも検査する。

ハードウェアのRGB・スクロールsetterはスタブで、マウスボタンはテスト用の
QMK Mouse Keys経路を使用する。実機のPointing Device経路やアプリ上の選択動作は別途確認する。
再現コマンドはキーマップREADMEの回帰テスト手順を参照。

## 優先度3: 状態の可視化

既存のOLEDとRGBを利用し、次の状態を簡潔に表示する。

| 状態 | 表示案 |
|---|---|
| Mark有効 | OLEDに `M` または `MARK` |
| `C-x` 入力待ち | OLEDに `CX`、または専用RGB色 |
| スクロール中 | OLEDに `S`、またはスクロール方向を表示 |

Auto Mouseは廃止済みのため表示改善の対象外とする。
専用Mouseレイヤーを将来採用する場合は、専用RGB色を検討する。

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

Auto Mouse廃止で回収した容量を踏まえて追加を検討し、
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

1. 自動NumFnを使用しない方針を確認する（完了）
2. Auto Mouseを無効化する（実装済み、実機確認待ち）
3. Mark処理をweak modifier化する（実装・QMK回帰テスト済み、実機確認待ち）
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
