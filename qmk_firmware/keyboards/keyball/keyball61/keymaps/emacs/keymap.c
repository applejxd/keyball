/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

#include "quantum.h"

enum custom_keycodes {
    CUT_LINE = KEYBALL_SAFE_RANGE,
    SET_MARK,   
    ABORT
};

/* ------ */
/* Macros */
/* ------ */

// see https://docs.qmk.fm/reference_keymap_extras#header-files
#include <sendstring_japanese.h>

bool set_mark_active = false;  // マーク状態を保持
uint8_t mod_state;
// see https://docs.qmk.fm/feature_macros#using-macros-in-c-keymaps
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    mod_state = get_mods();
    switch (keycode) {
        case CUT_LINE:
            if (record->event.pressed) {
                tap_code16(S(KC_END));
                SEND_STRING(SS_DELAY(10));
                tap_code16(C(KC_X));
            }
            break;
        case SET_MARK:
            if (record->event.pressed) set_mark_active = !set_mark_active;
            break;
        case ABORT:
            if (record->event.pressed) {
                if (set_mark_active) {
                    // マーク解除時は ESC を送信しない
                    set_mark_active = false;
                } else {
                    tap_code(KC_ESC);
                }
            }
            break;
        case KC_LEFT: case KC_RIGHT: case KC_UP: case KC_DOWN: 
        case KC_HOME: case KC_END: case KC_PGDN: case KC_PGUP:
            if (set_mark_active) {
                if (record->event.pressed) {
                    register_code(KC_LSFT);
                } else {
                    unregister_code(KC_LSFT);
                }
            }
            break;
        case C(KC_C): case C(KC_X): case C(KC_V): case C(KC_K): case KC_DEL:
            // 選択範囲を用いたアクションの後は選択解除
            if (record->event.pressed) { set_mark_active = false; }
            break;
        case KC_W:
            if (record->event.pressed) {
                if (mod_state & MOD_MASK_ALT) {
                    del_mods(MOD_MASK_ALT);
                    tap_code16(C(KC_C));    // w/o alt key
                    set_mods(mod_state);
                    set_mark_active = false;
                    return false;
                }
            }
            return true;
    }
    return true;
}

/* -------- */
/* Override */
/* -------- */

// ALT key override
// see https://docs.qmk.fm/features/key_overrides
// see https://docs.qmk.fm/feature_advanced_keycodes
#if defined(KEY_OVERRIDE_ENABLE) 
const key_override_t alt_v_to_pageup = ko_make_basic(MOD_MASK_ALT, KC_V, KC_PGUP);

// ALT+B -> CTRL+LEFT
const key_override_t alt_b_to_ctrl_left = ko_make_basic(MOD_MASK_ALT, KC_B, C(KC_LEFT));
// ALT+F -> CTRL+RIGHT
const key_override_t alt_f_to_ctrl_right = ko_make_basic(MOD_MASK_ALT, KC_F, C(KC_RIGHT));
// ALT+Y -> Win(GUI)+V
const key_override_t alt_w_to_gui_v = ko_make_basic(MOD_MASK_ALT, KC_Y, LGUI(KC_V));

const key_override_t **key_overrides = (const key_override_t *[]){
    &alt_v_to_pageup,
    &alt_b_to_ctrl_left,
    &alt_f_to_ctrl_right,
    &alt_w_to_gui_v,
    NULL
};
#endif // KEY_OVERRIDE_ENABLE

/* ----- */
/* Combo */
/* ----- */

#if defined(COMBO_ENABLE) 
const uint16_t PROGMEM left_click_combo[] = {KC_J, KC_K, COMBO_END};
const uint16_t PROGMEM right_click_combo[] = {KC_K, KC_L, COMBO_END};
const uint16_t PROGMEM middle_click_combo[] = {KC_J, KC_L, COMBO_END};
const uint16_t PROGMEM prev_click_combo[] = {KC_M, KC_COMM, COMBO_END};
const uint16_t PROGMEM next_click_combo[] = {KC_COMM, KC_DOT, COMBO_END};

combo_t key_combos[] = {
    COMBO(left_click_combo, KC_BTN1),
    COMBO(right_click_combo, KC_BTN2),
    COMBO(middle_click_combo, KC_BTN3),
    COMBO(prev_click_combo, KC_BTN4),
    COMBO(next_click_combo, KC_BTN5),
};
#endif // COMBO_ENABLE

/* ------ */
/* Layers */
/* ------ */

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  // 無効キーは右手トラックボールレイアウトでは存在しないキー
  [0] = LAYOUT_universal(
   S(KC_LBRC), KC_1     , KC_2     , KC_3     , KC_4     , KC_5     ,                                  KC_6     , KC_7     , KC_8     , KC_9     , KC_0     , S(KC_MINS)  ,
    KC_TAB   , KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                                  KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     , KC_MINS,
    MO(4)    , KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                                  KC_H     , KC_J     , KC_K     , KC_L     , LT(3,KC_SCLN), S(KC_7)  ,
    KC_LSFT  , KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     , KC_RBRC  ,              KC_NUHS, KC_N     , KC_M     , KC_COMM  , KC_DOT   , LT(2,KC_SLSH), KC_INT1  ,
    KC_ESC   , KC_LCTL  , KC_LALT  , KC_LGUI,LSFT_T(KC_LNG2),LCTL_T(KC_SPC),LT(3,KC_LNG1),       KC_BSPC,LT(2,KC_ENT),_______  ,_______   , _______  , KC_RALT  , MO(1)
  ),
  // US 配列(印字)を JP キーボード(ソフト設定)として使用
  [1] = LAYOUT_universal(
    S(KC_EQL), S(KC_1)   , KC_LBRC    , S(KC_3)  , S(KC_4)  , S(KC_5)  ,                                  KC_EQL   , S(KC_6)  ,S(KC_QUOT), S(KC_8)  , S(KC_9)   , S(KC_INT1),
    S(KC_TAB), S(KC_Q)   , S(KC_W)    , S(KC_E)  , S(KC_R)  , S(KC_T)  ,                                  S(KC_Y)  , S(KC_U)  , S(KC_I)  , S(KC_O)  , S(KC_P)   , S(KC_SCLN),
    S(KC_LCTL),S(KC_A)   , S(KC_S)    , S(KC_D)  , S(KC_F)  , S(KC_G)  ,                                  S(KC_H)  , S(KC_J)  , S(KC_K)  , S(KC_L)  , KC_QUOT   , S(KC_2)  ,
    _______  , S(KC_Z)   , S(KC_X)    , S(KC_C)  , S(KC_V)  , S(KC_B)  ,S(KC_RBRC),           S(KC_NUHS), S(KC_N)  , S(KC_M)  ,S(KC_COMM), S(KC_DOT), S(KC_SLSH), S(KC_INT3),
    S(KC_ESC), S(KC_LCTL), S(KC_LALT) , S(KC_LGUI), _______ , S(KC_SPC), _______  ,           S(KC_BSPC), S(KC_ENT), _______  , _______  , _______  , S(KC_RALT), _______
  ),
  // FN (TOP)・テンキー(左)・記号レイヤー(右)レイヤー。最左列はスクロール方向を変更。
  [2] = LAYOUT_universal(
    _______  , KC_F1    , KC_F2    , KC_F3    , KC_F4    , KC_F5    ,                                  KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   , KC_F11   ,
    _______  ,S(KC_QUOT), KC_7     , KC_8     , KC_9     , S(KC_8)  ,                                  S(KC_9)  , S(KC_1)  , S(KC_6)  , KC_LBRC  , S(KC_4)  , KC_F12   ,
    _______  ,S(KC_SCLN), KC_4     , KC_5     , KC_6     , KC_RBRC  ,                                  KC_NUHS  , KC_MINS  , S(KC_EQL), S(KC_3)  , KC_QUOT  , S(KC_2)  ,
    _______  ,S(KC_MINS), KC_1     , KC_2     , KC_3     ,S(KC_RBRC), S(KC_8)  ,            S(KC_9)  , S(KC_NUHS),S(KC_INT1), KC_EQL   ,S(KC_LBRC),S(KC_SLSH),S(KC_INT3),
    _______  , _______  , KC_0     , KC_DOT   , _______  , _______  , _______  ,             KC_DEL  , _______  , _______  , _______  , _______  , _______  , _______
  ),
  // Fn, 設定レイヤー
  // SSNP, CPI, SCRL, KBC (see https://github.com/Yowkees/keyball/blob/main/qmk_firmware/keyboards/keyball/lib/keyball/keycodes.md)
  // QK_BOOT, EE_CLR (see https://docs.qmk.fm/quantum_keycodes#qmk-keycodes)
  [3] = LAYOUT_universal(
    _______  , _______  , _______  , _______  , _______  , _______  ,                                  _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , SSNP_FRE , KC_F7    , KC_F8    , KC_F9    , KC_F12   ,                                  AML_TO   , AML_I50  , AML_D50  , EE_CLR   , KBC_RST  , _______  ,
    _______  , SSNP_VRT , KC_F4    , KC_F5    , KC_F6    , KC_F11   ,                                  CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , KBC_SAVE , _______  ,
    _______  , SSNP_HOR , KC_F1    , KC_F2    , KC_F3    , KC_F10   , _______  ,            _______  , SCRL_DVD , SCRL_DVI , SCRL_MO  , SCRL_TO  , QK_BOOT  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  , _______  ,            _______  , _______  , _______  , _______  , _______  , _______  , _______
  ),
  // Emacs レイヤー (C-)
  [4] = LAYOUT_universal(
    _______  , KC_F1       , KC_F2    , KC_F3    , KC_F4    , KC_F5    ,                                  KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   , KC_F11   ,
    C(KC_TAB), G(S(KC_F23)), C(KC_X)  , KC_END   , C(KC_R)  , C(KC_T)  ,                                  C(KC_V)  , C(KC_Z)  , KC_TAB   , C(KC_O)  , KC_UP    , KC_F12  ,
    _______  , KC_HOME     , C(KC_F)  , KC_DEL   , KC_RGHT  , ABORT    ,                                  KC_BSPC  , KC_ENT   , CUT_LINE , C(KC_L)  ,C(KC_SCLN), _______  ,
    C(KC_LSFT),G(KC_DOWN)  , OSL(5)   , C(KC_C)  , KC_PGDN  , KC_LEFT  , C(KC_RBRC),          C(KC_NUHS), KC_DOWN  , KC_ENT   ,C(KC_COMM),C(KC_DOT) , C(KC_Z)  ,C(KC_RSFT) ,
    C(KC_ESC), _______     ,C(KC_LALT),C(KC_LGUI), _______  , SET_MARK , G(S(KC_S))  ,        C(KC_BSPC), C(KC_ENT), _______  , _______  , _______  ,C(KC_RALT), _______ 
  ),
  // Emacs レイヤー (C-x)
  [5] = LAYOUT_universal(
    _______  , _______   , _______    , _______  , _______  , _______  ,                                  _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______   , S(C(KC_S)) , _______  , _______  , _______  ,                                  _______  , C(KC_Z)  , _______  , A(KC_TAB), _______  , _______  ,
    _______  , _______   , C(KC_S)    , G(KC_E)  , C(KC_O)  , _______  ,                                  C(KC_A)  , _______  , C(KC_W)  , _______  , _______  , _______  ,
    _______  , _______   , _______    , A(KC_F4) , _______  , G(KC_TAB), _______ ,             _______  , _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______   , _______    , _______  , _______  , _______  , _______ ,             _______  , _______  , _______  , _______  , _______  , _______  , _______ 
  ),
};

// clang-format on

layer_state_t layer_state_set_user(layer_state_t state) {
    int layer_num = get_highest_layer(state);

    if (layer_num == 2) {
        // レイヤー2では水平スクロール
        keyball_set_scroll_mode(true);
        keyball_set_scrollsnap_mode(KEYBALL_SCROLLSNAP_MODE_HORIZONTAL);
    } else if (layer_num == 3) {
        // レイヤー3では垂直スクロール
        keyball_set_scroll_mode(true);
        keyball_set_scrollsnap_mode(KEYBALL_SCROLLSNAP_MODE_VERTICAL);
    } else {
        // 他のレイヤーではスクロールを無効化
        keyball_set_scroll_mode(false);
    }
    
    return state;
}

#ifdef OLED_ENABLE

#include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif
