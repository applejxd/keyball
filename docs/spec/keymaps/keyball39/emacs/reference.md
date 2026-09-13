# keyball39 emacs 自動生成リファレンス

> `keymap.c`、`config.h`、`rules.mk`、`via.json` と `metadata.yaml` から
> 自動生成される。直接編集せず、`mise run keymap:generate` で更新する。

## 前提

| 項目 | 値 |
|---|---|
| OS | Windows |
| 配列 | JIS |
| トラックボール構成 | Right |

### 注意

- KC_LNG1、KC_LNG2、KC_INT1、KC_INT3はJIS配列を前提とする
- Win、Alt+F4、Alt+TabなどWindows向けショートカットを含む

## レイヤー

| 番号 | 識別子 | 表示名 | 用途 |
|---|---|---|---|
| 0 | KL_BASE | Base | 通常の文字入力 |
| 1 | KL_SMB | Symbols | 記号入力と垂直スクロール |
| 2 | KL_NUMFN | Num/Fn | 左手テンキーと右手ファンクションキー |
| 3 | KL_EMACS | Emacs | Emacs形式の編集・移動操作と水平スクロール |
| 4 | KL_CX | C-x | C-x prefixに続く操作 |

## レイヤー操作

| 元レイヤー | キー | 動作 | 移動先 |
|---|---|---|---|
| Base | P | Hold | Emacs |
| Base | ; | Hold | Symbols |
| Base | Space | Hold | Symbols |
| Base | Kana | Hold | Emacs |
| Base | Enter | Hold | Num/Fn |
| Emacs | C-x | One shot | C-x |

## Tap / Hold

| レイヤー | Tap | Hold |
|---|---|---|
| Base | P | Emacs |
| Base | ; | Symbols |
| Base | Z | Shift |
| Base | / | Shift |
| Base | Eisu | Shift |
| Base | Space | Symbols |
| Base | Kana | Emacs |
| Base | Enter | Num/Fn |

## Combo

| 入力 | 出力 |
|---|---|
| J + K | Mouse 1 |
| K + L | Mouse 2 |
| J + L | Mouse 3 |
| M + , | Back |
| , + . | Forward |

## Key Override

| 入力 | 出力 |
|---|---|
| Alt+V | PgUp |
| Alt+B | Ctrl+Left |
| Alt+F | Ctrl+Right |
| Alt+Y | Win+V |

## カスタムキー

| 識別子 | 表示名 | 動作 |
|---|---|---|
| CUT_LINE | Cut line | カーソル位置から行末まで選択して切り取る |
| SET_MARK | Set mark | Mark選択モードの有効・無効を切り替える |
| ABORT | Abort | Mark中は選択を解除し、それ以外ではEscを送信する |

## RGBレイヤー色

| レイヤー | 色 |
|---|---|
| Base | White |
| Symbols | Azure |
| Num/Fn | Blue |
| Emacs | Purple |
| C-x | Magenta |

## Auto Mouse

| 状態 | 対象レイヤー |
|---|---|
| Disabled | - |

## ビルド機能

| 機能 | 状態 |
|---|---|
| LTO_ENABLE | Enabled |
| BOOTMAGIC_ENABLE | Disabled |
| EXTRAKEY_ENABLE | Disabled |
| CONSOLE_ENABLE | Disabled |
| COMMAND_ENABLE | Disabled |
| NKRO_ENABLE | Disabled |
| BACKLIGHT_ENABLE | Disabled |
| AUDIO_ENABLE | Disabled |
| POINTING_DEVICE_ENABLE | Enabled |
| MOUSEKEY_ENABLE | Disabled |
| RGBLIGHT_ENABLE | Enabled |
| RGB_MATRIX_ENABLE | Disabled |
| SLEEP_LED_ENABLE | Disabled |
| OLED_ENABLE | Enabled |
| SPACE_CADET_ENABLE | Disabled |
| GRAVE_ESC_ENABLE | Disabled |
| MAGIC_ENABLE | Disabled |
| KEY_OVERRIDE_ENABLE | Enabled |
| COMBO_ENABLE | Enabled |
| VIA_ENABLE | Disabled |
| MUSIC_ENABLE | Disabled |

## 外部ショートカット

| キー | 用途 |
|---|---|
| Win+Shift+F23 | 外部アプリ側の割り当てに依存するショートカット（用途未記録） |

## 状態遷移・操作フロー

### Mark

1. Set markで選択モードを切り替える
2. 選択モード中にカーソル移動キーを押すとShift選択になる
3. C、Ctrl+C、Ctrl+X、Ctrl+V、Deleteの押下後は選択モードを解除する
4. Abortは選択モードを解除し、もう一度押すとEscとして動作する

### C-x

1. EmacsレイヤーのC-xキーでワンショットのC-xレイヤーへ移動する
2. 次の1キーを処理するとC-xレイヤーを解除し、元のアクティブレイヤーへ戻る
