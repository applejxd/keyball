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
        case KC_C: case C(KC_C): case C(KC_X): case C(KC_V): case KC_DEL:
            // 選択範囲を用いたアクションの後は選択解除
            if (record->event.pressed) { set_mark_active = false; }
            break;
        // case KC_W:
        //     if (record->event.pressed) {
        //         if (mod_state & MOD_MASK_ALT) {
        //             del_mods(MOD_MASK_ALT);
        //             tap_code16(C(KC_C));    // w/o alt key
        //             set_mods(mod_state);
        //             set_mark_active = false;
        //             return false;
        //         }
        //     }
        //     return true;
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
  // アルファベットレイヤー
  // 無効キーは右手トラックボールレイアウトでは存在しないキー
  [0] = LAYOUT_universal(
    KC_Q          , KC_W     , KC_E     , KC_R     , KC_T     ,                          KC_Y     , KC_U     , KC_I     , KC_O     , KC_P         , 
    KC_A          , KC_S     , KC_D     , KC_F     , KC_G     ,                          KC_H     , KC_J     , KC_K     , KC_L     , LT(1,KC_SCLN), 
    KC_Z          , KC_X     , KC_C     , KC_V     , KC_B     ,                          KC_N     , KC_M     , KC_COMM  , KC_DOT   , LT(3,KC_SLSH), 
    KC_LCTL       , KC_LALT  , KC_LGUI,LSFT_T(KC_LNG2),LT(1,KC_SPC),LT(3,KC_LNG1), KC_BSPC,LT(2,KC_ENT),_______ , _______  , _______  , KC_RSFT    
  ),
  // 記号専用レイヤー
  [1] = LAYOUT_universal(
    S(KC_1)    , KC_LBRC  , S(KC_3)    , S(KC_4)    , S(KC_8)    ,                     S(KC_9)    , S(KC_6)    , S(KC_QUOT) , S(KC_7)    , S(KC_2)    ,
    S(KC_LBRC) , KC_EQL   , S(KC_INT3) , S(KC_SCLN) , KC_RBRC    ,                     KC_NUHS    , KC_MINS    , S(KC_EQL)  , S(KC_MINS) , KC_QUOT    ,
    XXXXXXX    , XXXXXXX  , S(KC_5)    , KC_INT1    , S(KC_RBRC) ,                     S(KC_NUHS) , S(KC_INT1) , S(KC_COMM) , S(KC_DOT)  , S(KC_SLSH) ,
    _______    , _______  , _______    , KC_LSFT    , _______    , _______ ,  _______, _______    , _______    , _______    , _______    , _______   
  ),
  // FN (TOP)・テンキー(左)・記号レイヤー(右)レイヤー。最左列はスクロール方向を変更。
  [2] = LAYOUT_universal(
    S(KC_QUOT), KC_7     , KC_8     , KC_9     , _______   ,                          _______  , KC_F7   , KC_F8    , KC_F9    , KC_F12   , 
    S(KC_SCLN), KC_4     , KC_5     , KC_6     , _______   ,                          _______  , KC_F4   , KC_F5    , KC_F6    , KC_F11   , 
    S(KC_MINS), KC_1     , KC_2     , KC_3     , _______   ,                          _______  , KC_F1   , KC_F2    , KC_F3    , KC_F10   , 
     _______  , KC_0     , KC_DOT   , KC_LSFT  , _______   , _______  ,     _______ , _______  , _______ , _______  , _______  , _______  
  ),
  // Emacs レイヤー (C-)
  [3] = LAYOUT_universal(
    G(S(KC_F23)), C(KC_X)  , KC_END   , C(KC_R)  , C(KC_T)  ,                           C(KC_V)  , C(KC_Z)  , KC_TAB   , C(KC_O)  , KC_UP    , 
    KC_HOME     , C(KC_F)  , KC_DEL   , KC_RGHT  , ABORT    ,                           KC_BSPC  , KC_ENT   , CUT_LINE , C(KC_L)  ,C(KC_SCLN), 
    G(KC_DOWN)  , OSL(4)   , C(KC_C)  , KC_PGDN  , KC_LEFT  ,                           KC_DOWN  , KC_ENT   ,C(KC_COMM),C(KC_DOT) , C(KC_Z)  ,
    _______     ,C(KC_LALT),C(KC_LGUI), _______  , SET_MARK , _______  ,    G(S(KC_S)), SET_MARK , _______  , _______  , _______  ,C(KC_RALT)
  ),
  // Emacs レイヤー (C-x)
  [4] = LAYOUT_universal(
    _______    , S(C(KC_S)) , _______  , _______  , _______  ,                                  _______  , C(KC_Z)  , _______  , A(KC_TAB), _______  , 
    _______    , C(KC_S)    , G(KC_E)  , C(KC_O)  , _______  ,                                  C(KC_A)  , _______  , C(KC_W)  , _______  , _______  , 
    _______    , _______    , A(KC_F4) , _______  , G(KC_TAB),                                  _______  , _______  , _______  , _______  , _______  , 
    _______    , _______    , _______  , _______  , _______  , _______ ,             _______  , _______  , _______  , _______  , _______  , _______  
  ),
};

// clang-format on

layer_state_t layer_state_set_user(layer_state_t state) {
    int heighest_layer_num = get_highest_layer(state);

    if (heighest_layer_num == 1) {
        // 垂直スクロール
        keyball_set_scroll_mode(true);
        keyball_set_scrollsnap_mode(KEYBALL_SCROLLSNAP_MODE_VERTICAL);
    } else if (heighest_layer_num == 3) {
        // 水平スクロール
        keyball_set_scroll_mode(true);
        keyball_set_scrollsnap_mode(KEYBALL_SCROLLSNAP_MODE_HORIZONTAL);
    } else {
        // 他のレイヤーではスクロールを無効化
        keyball_set_scroll_mode(false);
    }

    uint8_t layer = biton32(state);
    switch(layer) {
        case 0:
            rgblight_sethsv(HSV_WHITE);
            break;
        case 1:
            rgblight_sethsv(HSV_RED);
            break;
        case 2:
            rgblight_sethsv(HSV_BLUE);
            break;
        case 3:
            rgblight_sethsv(HSV_GREEN);
            break;
        case 4:
            rgblight_sethsv(HSV_YELLOW);
            break;
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
