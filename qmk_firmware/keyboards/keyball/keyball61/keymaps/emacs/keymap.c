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

You should have recei
ved a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

#include "quantum.h"

enum custom_keycodes {
    CUT_LINE = SAFE_RANGE,  // cutline as Emacs
    SET_MARK,   
    UNMARK,
    COPY_TEXT,  
    CUT_TEXT,
    // for Windows
    DEEPL
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
        case UNMARK:
            if (record->event.pressed) {
                tap_code(KC_ESC);
                set_mark_active = false;
            }
            break;
        case COPY_TEXT:
            if (record->event.pressed) {
                tap_code16(C(KC_C));
                set_mark_active = false;
            }
            break;
        case CUT_TEXT:
            if (record->event.pressed) {
                tap_code16(C(KC_X));
                set_mark_active = false;
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
        case DEEPL:
            if (record->event.pressed) {
                tap_code16(G(KC_R));
                // wait for moving active window and clear inputs
                SEND_STRING(SS_DELAY(300));
                tap_code(KC_DEL);
                SEND_STRING(
                    SS_DELAY(100)
                    // run command
                    // "\%UserProfile\%/src/windows-setup/bin/deepl.bat"
                    "powershell -Command "
                    SS_DELAY(100)
                    "\"Start-Process (\'https://www.deepl.com/translator#en/ja/\'"
                    SS_DELAY(100)
                    " + [uri]::EscapeDataString((Get-Clipboard)))\""
                    SS_DELAY(100)
                );
                tap_code(KC_ENT);
            }
            break;
    }
    return true;
}

/* -------- */
/* Override */
/* -------- */

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

/* ------ */
/* Layers */
/* ------ */

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT_universal(
    KC_ESC   , KC_1     , KC_2     , KC_3     , KC_4     , KC_5     ,                                  KC_6     , KC_7     , KC_8     , KC_9     , KC_0     , KC_MINS  ,
    KC_TAB   , KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                                  KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     , KC_INT3  ,
    MO(4)    , KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                                  KC_H     , KC_J     , KC_K     , KC_L     , KC_SCLN  , S(KC_7)  ,
    MO(1)    , KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     , KC_RBRC  ,              KC_NUHS, KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  , KC_RSFT  ,
    _______  , KC_LCTL  , KC_LALT  , KC_LGUI,LT(1,KC_LNG2),LT(2,KC_SPC),LT(3,KC_LNG1),    KC_BSPC,LT(2,KC_ENT),LT(1,KC_LNG2),KC_RGUI, _______ , KC_RALT  , KC_PSCR
  ),

  [1] = LAYOUT_universal(
    S(KC_ESC), S(KC_1)   , KC_LBRC    , S(KC_3)  , S(KC_4)  , S(KC_5)  ,                                  KC_EQL   , S(KC_6)  ,S(KC_QUOT), S(KC_8)  , S(KC_9)  ,S(KC_INT1),
    S(KC_DEL), S(KC_Q)   , S(KC_W)    , S(KC_E)  , S(KC_R)  , S(KC_T)  ,                                  S(KC_Y)  , S(KC_U)  , S(KC_I)  , S(KC_O)  , S(KC_P)  ,S(KC_INT3),
    S(KC_TAB), S(KC_A)   , S(KC_S)    , S(KC_D)  , S(KC_F)  , S(KC_G)  ,                                  S(KC_H)  , S(KC_J)  , S(KC_K)  , S(KC_L)  , KC_QUOT  , S(KC_2)  ,
    _______  , S(KC_Z)   , S(KC_X)    , S(KC_C)  , S(KC_V)  , S(KC_B)  ,S(KC_RBRC),           S(KC_NUHS), S(KC_N)  , S(KC_M)  ,S(KC_COMM), S(KC_DOT),S(KC_SLSH),S(KC_RSFT),
    _______  , S(KC_LCTL), S(KC_LALT) , S(KC_LGUI), _______  , _______  , _______  ,            _______  , _______  , _______  ,S(KC_RGUI), _______  , S(KC_RALT), _______
  ),

  [2] = LAYOUT_universal(
    SSNP_FRE , KC_F1    , KC_F2    , KC_F3    , KC_F4    , KC_F5    ,                                  KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   , KC_F11   ,
    SSNP_VRT , _______  , KC_7     , KC_8     , KC_9     , _______  ,                                  _______  , KC_LEFT  , KC_UP    , KC_RGHT  , _______  , KC_F12   ,
    SSNP_HOR , _______  , KC_4     , KC_5     , KC_6     ,S(KC_SCLN),                                  KC_PGUP  , KC_BTN1  , KC_DOWN  , KC_BTN2  , KC_BTN3  , _______  ,
    _______  , _______  , KC_1     , KC_2     , KC_3     ,S(KC_MINS), S(KC_8)  ,            S(KC_9)  , KC_PGDN  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , KC_0     , KC_DOT   , _______  , _______  , _______  ,             KC_DEL  , _______  , _______  , _______  , _______  , _______  , _______
  ),

  [3] = LAYOUT_universal(
    RGB_TOG  , AML_TO   , AML_I50  , AML_D50  , _______  , _______  ,                                  RGB_M_P  , RGB_M_B  , RGB_M_R  , RGB_M_SW , RGB_M_SN , RGB_M_K  ,
    RGB_MOD  , RGB_HUI  , RGB_SAI  , RGB_VAI  , _______  , _______  ,                                  RGB_M_X  , RGB_M_G  , RGB_M_T  , RGB_M_TW , _______  , _______  ,
    RGB_RMOD , RGB_HUD  , RGB_SAD  , RGB_VAD  , _______  , _______  ,                                  CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , KBC_SAVE , KBC_RST  ,
    _______  , _______  , SCRL_DVD , SCRL_DVI , SCRL_MO  , SCRL_TO  , EE_CLR   ,            EE_CLR   , KC_HOME  , KC_PGDN  , KC_PGUP  , KC_END   , _______  , _______  ,
    QK_BOOT  , _______  , KC_LEFT  , KC_DOWN  , KC_UP    , KC_RGHT  , _______  ,            _______  , KC_BSPC  , _______  , _______  , _______  , _______  , QK_BOOT
  ),

  [4] = LAYOUT_universal(
    _______  , _______     , _______  , _______  , _______  , _______  ,                                  _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , G(S(KC_F23)), CUT_TEXT , KC_END   , C(KC_R)  , C(KC_T)  ,                                  C(KC_V)  , C(KC_Z)  , KC_TAB   , C(KC_O)  , KC_UP    , _______  ,
    _______  , KC_HOME     , C(KC_F)  , KC_DEL   , KC_RGHT  , UNMARK   ,                                  KC_BSPC  , KC_ENT   , CUT_LINE , C(KC_L)  , _______  , _______  ,
    _______  , G(KC_DOWN)  , OSL(5)   , C(KC_C)  , KC_PGDN  , KC_LEFT  , _______ ,             _______  , KC_DOWN  , KC_ENT   , _______  , _______  , C(KC_Z)  , _______  ,
    _______  , _______     , _______  , _______  , _______  , SET_MARK , _______  ,            _______  , _______  , _______  , _______  , _______  , _______  , _______ 
  ),

  [5] = LAYOUT_universal(
    _______  , _______   , _______    , _______  , _______  , _______  ,                                  _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______   , S(C(KC_S)) , _______  , _______  , _______  ,                                  _______  , C(KC_X)  , _______  , A(KC_TAB), _______  , _______  ,
    _______  , _______   , C(KC_S)    , G(KC_E)  , C(KC_O)  , _______  ,                                  C(KC_A)  , _______  , C(KC_W)  , _______  , _______  , _______  ,
    _______  , _______   , _______    , A(KC_F4) , _______  , G(KC_TAB), _______ ,             _______  , _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______   , _______    , _______  , _______  , _______  , _______ ,             _______  , _______  , _______  , _______  , _______  , _______  , _______ 
  ),
};

// clang-format on

layer_state_t layer_state_set_user(layer_state_t state) {
    // Auto enable scroll mode when the highest layer is 3
    keyball_set_scroll_mode(get_highest_layer(state) == 3);
    return state;
}

#ifdef OLED_ENABLE

#    include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif
