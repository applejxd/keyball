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
| 5 | KL_GAME | Game | SymbolsとNum/Fnの同時長押し中だけ8方向移動（試験用） |

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
| Alt+V（Mark中） | Shift+PgUp |
| Alt+B（Mark中） | Ctrl+Shift+Left |
| Alt+F（Mark中） | Ctrl+Shift+Right |
| Alt+Shift+,（Mark中） | Ctrl+Shift+Home |
| Alt+Shift+.（Mark中） | Ctrl+Shift+End |
| Alt+<（Mark中） | Ctrl+Shift+Home |
| Alt+>（Mark中） | Ctrl+Shift+End |
| Alt+V（Mark OFF） | PgUp |
| Alt+B（Mark OFF） | Ctrl+Left |
| Alt+F（Mark OFF） | Ctrl+Right |
| Alt+Shift+,（Mark OFF） | Ctrl+Home |
| Alt+Shift+.（Mark OFF） | Ctrl+End |
| Alt+<（Mark OFF） | Ctrl+Home |
| Alt+>（Mark OFF） | Ctrl+End |
| Alt+Y | Win+V |
| Alt+W | Copy region |
| Alt+D | Cut word |
| Alt+BS | Cut prev word |
| Alt+@ | Mark word |

## カスタムキー

| 識別子 | 表示名 | 動作 |
|---|---|---|
| CUT_LINE | Cut line | カーソル位置から行末まで選択して切り取る |
| SET_MARK | Set mark | Mark選択モードの有効・無効を切り替える |
| ABORT | Abort | Mark中は選択モードだけを解除し、それ以外ではEscを送信する |
| COPY_REGION | Copy region | Ctrl+Cで選択範囲をコピーし、Markを終了する |
| CUT_WORD | Cut word | Ctrl+Shift+Rightで選択してCtrl+Xで切り取り、Markを終了する |
| CUT_WORD_BACKWARD | Cut prev word | Ctrl+Shift+Leftで選択してCtrl+Xで切り取り、Markを終了する |
| MARK_WORD | Mark word | Ctrl+Shift+Rightで次の単語を選択し、Markを有効にする |

## RGBレイヤー色

| レイヤー | 色 |
|---|---|
| Base | White |
| Symbols | Azure |
| Num/Fn | Blue |
| Emacs | Purple |
| C-x | Magenta |
| Game | Green |

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

### Game

1. SpaceとEnterを両方長押しするとGameになり、どちらかを離すと解除する。押す順序は自由
2. SymbolsとNum/Fnの同時有効化で判定するため、Spaceの代わりにセミコロン長押しでも有効になる。ただし、その間は同じ位置のF11を押せない
3. 左手の7/8/9位置はHome/Up/Page Up、4/6位置はLeft/Right、1/2/3位置はEnd/Down/Page Downを送る
4. 中央の5位置は誤操作防止で無効。NumLock非依存の待機キーは今回割り当てない
5. Enterを保持してSpaceを離せば通常数字に戻る。移動キーを離してから数字を押し直す
6. 右手のF1〜F12はNum/Fnと同じ配置を維持する。NumLockは参照・変更しない
7. Gameに入るとMarkと選択用Shiftを解除し、抜けてもMarkは復元しない。物理Shiftは維持する
8. トラックボールはポインター移動、RGBは緑。アプリの自動判別やレイヤーの固定は行わない

### Combo

1. 通常入力のロール誤発火を減らすため、短い受付時間と最初の候補キーを起点とするタイマーを使う（設定値はキーマップのconfig.hを参照）
2. 後続の候補キーで受付時間を延長しない。タイマーは別の組の候補キーにも共有される
3. 押す順序は自由で、2キーを重ねて押すとクリックし、両方を離すまでボタンを保持するためドラッグできる
4. 短いクリック・連続クリック・物理修飾キーとの併用は維持するが、受付時間内のロールはクリックになり得る
5. 各レイヤーの実際のキーコードで判定する。C-x待機中も下位レイヤーへ透過するキーは従来どおりComboになり得る

### Mark

1. Set markでキーボード内部の選択モードを切り替える（Emacs固有のコマンドは送信しない）
2. 選択モード中は矢印、Home、End、Page Up、Page Downに補助Shiftを加え、対応するアプリで範囲選択する
3. 移動キーを離すと補助Shiftを解除するが、選択モードは維持されるため連打でも選択を続けられる
4. 移動キーを重ねて押した場合は、最後の対象キーを離すまで補助Shiftを維持する
5. 物理Shiftとは独立に管理し、Markを終了しても自分で押しているShiftは解除しない
6. Markの補助にはweak右Shiftを使い、既存のShift付き記号・マクロのweak左Shiftと分離する
7. Ctrl+C、Ctrl+X、Ctrl+Vはレイヤー定義・左右の物理Ctrl・ワンショットCtrlのいずれでも選択モードを解除する（Shift併用可、Alt・GUI併用は対象外）
8. 修飾なしのC/X/Vでは選択モードを解除しない。Deleteの終了動作は従来どおり
9. Abortは選択モードとMarkの補助Shiftだけを解除し、もう一度押すとEscを送信する（画面上の選択範囲を直接消す操作ではない）
10. MarkがONでも移動キーを押していなければ文字やクリックに補助Shiftは付かないが、移動キーとの同時操作にはShiftが作用し得る
11. Alt+B/F/VとAlt+<、Alt+>もMark中はShift選択になる。Win+Downなど、それ以外のModifier付き移動は対象外
12. Alt移動の選択用ShiftはQMKのOverride専用修飾として管理し、通常移動のweak右Shiftや物理Shiftと分離する
13. Alt+WのコピーとAlt+D/Backspaceの単語切り取りでMarkを終了し、Alt+@の単語選択でMarkを有効にする

### Macros

1. Cut lineはShift+End、10 ms待機、Ctrl+Xの順に送信し、MarkがOFFのAbortは修飾なしのEscを送信する
2. 実行中だけ物理・補助修飾キーを外し、終了後に復元する。マウスボタンの保持状態は変更しない
3. 待機中のワンショット修飾はマクロが消費し、マクロにも次のキーにも付加しない
4. 有効中・遅延中のKey Overrideは出力前に解除する。C-xワンショットでもマクロは1回だけ実行する
5. Cut lineはMarkを終了せず、長押しリピート・行末での改行削除・kill ringへの連結は行わない
6. 単語切り取りはCtrl+Shift+Right/Left、10 ms待機、Ctrl+Xの順で送信する。単語境界はWindowsアプリに依存する
7. コピー・単語切り取り・単語選択はAltを先に押し、文字キー押下時に1回実行する。解放時や長押しでは再実行しない

### Alt Override

1. 左右どちらのAltでも発火する。Alt+B/F/V/Yは物理Shiftを併用すると出力にもShiftが付く
2. Alt+<、Alt+>はCtrl+Home/Endへ変換し、入力側のShiftを取り除く。Mark中だけ選択用Shiftを付加する
3. Alt+<、Alt+>はBaseのShift+Comma/DotとSymbolsの<、>の両方で使用できる
4. Alt+@はJISのSymbolsレイヤーの@キーを使用する。Alt+Shift+2ではない
5. Ctrl・GUIの保持中は変換しない。Ctrl・GUIを離しただけでも変換を開始しない
6. 変換中にCtrl・GUIを押すと変換を終了する。Altを先に離した場合も元の文字は再送しない
7. 対象は全レイヤーだが、各レイヤーの実際のキーコードがトリガーに一致する場合のみ発火する
8. Mark ON/OFFの移動Overrideは相互排他的に有効化し、追加の修飾キー操作で選択用Shiftを失わないようにする

### C-x

1. EmacsレイヤーのC-xキーでワンショットのC-xレイヤーへ移動する
2. 次の1キーを処理するとC-xレイヤーを解除し、元のアクティブレイヤーへ戻る
