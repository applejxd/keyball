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

#include <string.h>

enum custom_keycodes {
    CUT_LINE = KEYBALL_SAFE_RANGE,
    SET_MARK,   
    ABORT
};

/* ------ */
/* Macros */
/* ------ */

bool set_mark_active = false;  // マーク状態を保持
static matrix_row_t mark_navigation_keys[MATRIX_ROWS];
static uint8_t mark_navigation_count;

// S(kc) and CUT_LINE use weak left Shift; reserve weak right Shift for Mark.
#define MARK_SHIFT MOD_BIT(KC_RSFT)

static void restore_mark_shift(void) {
    if (mark_navigation_count) {
        add_weak_mods(MARK_SHIFT);
    }
}

static void end_mark(bool send_report) {
    bool had_navigation = mark_navigation_count != 0;
    set_mark_active = false;
    memset(mark_navigation_keys, 0, sizeof(mark_navigation_keys));
    mark_navigation_count = 0;
    if (had_navigation) {
        del_weak_mods(MARK_SHIFT);
        if (send_report) {
            send_keyboard_report();
        }
    }
}

static bool ends_mark(uint16_t keycode) {
    if (keycode == KC_DEL) {
        return true;
    }

    uint8_t mods = get_mods() | get_weak_mods() | get_oneshot_mods();
    if (IS_QK_MODS(keycode)) {
        uint8_t key_mods = QK_MODS_GET_MODS(keycode) & 0x0f;
        mods |= (keycode & QK_RMODS_MIN) ? key_mods << 4 : key_mods;
        keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
    }

    return (keycode == KC_C || keycode == KC_X || keycode == KC_V) &&
           (mods & MOD_MASK_CTRL) && !(mods & (MOD_MASK_ALT | MOD_MASK_GUI));
}

static void send_edit_macro(uint16_t keycode) {
    uint8_t mods = get_mods();
    uint8_t weak_mods = get_weak_mods();
    clear_mods();
    clear_weak_mods();
    // A pending one-shot is consumed by the macro, not applied to the next key.
    clear_oneshot_mods();
    send_keyboard_report();

    if (keycode == CUT_LINE) {
        tap_code16(S(KC_END));
        wait_ms(10);
        tap_code16(C(KC_X));
    } else {
        tap_code(KC_ESC);
    }

    set_mods(mods);
    set_weak_mods(weak_mods);
    send_keyboard_report();
}

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
    // action_exec clears weak mods before Combo and tap-hold processing.
    restore_mark_shift();
    return true;
}

void housekeeping_task_user(void) {
    // Combo also clears weak mods after dispatching its buffered records.
    restore_mark_shift();
}

// see https://docs.qmk.fm/feature_macros#using-macros-in-c-keymaps
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // Track positions, not keycodes: late and synthetic releases are idempotent.
    if (IS_KEYEVENT(record->event) && !record->event.pressed) {
        matrix_row_t key = (matrix_row_t)1 << record->event.key.col;
        if (mark_navigation_keys[record->event.key.row] & key) {
            mark_navigation_keys[record->event.key.row] &= ~key;
            if (--mark_navigation_count == 0) {
                del_weak_mods(MARK_SHIFT);
            }
        }
    }
    restore_mark_shift();

    if (record->event.pressed && ends_mark(keycode)) {
        // Let the edit key send the report, so one-shot Ctrl reaches that key.
        end_mark(false);
    }

    switch (keycode) {
        case CUT_LINE:
        case ABORT:
            // Consume the event, but first let Key Override release its output.
#ifdef KEY_OVERRIDE_ENABLE
            process_key_override(keycode, record);
#endif
            if (record->event.pressed) {
                if (keycode == ABORT && set_mark_active) {
                    // マーク解除時は ESC を送信しない
                    end_mark(true);
                } else {
                    send_edit_macro(keycode);
                }
            }
            return false;
        case SET_MARK:
            if (record->event.pressed) {
                if (set_mark_active) {
                    end_mark(true);
                } else {
                    set_mark_active = true;
                }
            }
            break;
        case KC_LEFT: case KC_RIGHT: case KC_UP: case KC_DOWN: 
        case KC_HOME: case KC_END: case KC_PGDN: case KC_PGUP:
            if (set_mark_active && record->event.pressed && IS_KEYEVENT(record->event)) {
                matrix_row_t key = (matrix_row_t)1 << record->event.key.col;
                if (!(mark_navigation_keys[record->event.key.row] & key)) {
                    mark_navigation_keys[record->event.key.row] |= key;
                    ++mark_navigation_count;
                }
                restore_mark_shift();
            }
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
#define ALT_OVERRIDE_OPTIONS (ko_option_activation_trigger_down | ko_option_activation_required_mod_down | ko_option_no_reregister_trigger)

const key_override_t alt_v_to_pageup = ko_make_with_layers_negmods_and_options(MOD_MASK_ALT, KC_V, KC_PGUP, ~0, MOD_MASK_CG, ALT_OVERRIDE_OPTIONS);

// ALT+B -> CTRL+LEFT
const key_override_t alt_b_to_ctrl_left = ko_make_with_layers_negmods_and_options(MOD_MASK_ALT, KC_B, C(KC_LEFT), ~0, MOD_MASK_CG, ALT_OVERRIDE_OPTIONS);
// ALT+F -> CTRL+RIGHT
const key_override_t alt_f_to_ctrl_right = ko_make_with_layers_negmods_and_options(MOD_MASK_ALT, KC_F, C(KC_RIGHT), ~0, MOD_MASK_CG, ALT_OVERRIDE_OPTIONS);
// ALT+Y -> Win(GUI)+V
const key_override_t alt_y_to_gui_v = ko_make_with_layers_negmods_and_options(MOD_MASK_ALT, KC_Y, LGUI(KC_V), ~0, MOD_MASK_CG, ALT_OVERRIDE_OPTIONS);

const key_override_t **key_overrides = (const key_override_t *[]){
    &alt_v_to_pageup,
    &alt_b_to_ctrl_left,
    &alt_f_to_ctrl_right,
    &alt_y_to_gui_v,
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

enum keymap_layer {
    KL_BASE = 0,  // アルファベットレイヤー
    KL_SMB,       // 記号専用レイヤー
    KL_NUMFN,     // テンキー(左)・FN(右)レイヤー
    KL_EMACS,     // Emacs レイヤー (C-)
    KL_CX,        // Emacs レイヤー (C-x)
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  // アルファベットレイヤー
  // 無効キーは右手トラックボールレイアウトでは存在しないキー
  [KL_BASE] = LAYOUT_universal(
    KC_Q        , KC_W   , KC_E   , KC_R           , KC_T             ,                                KC_Y               , KC_U   , KC_I   , KC_O   , LT(KL_EMACS,KC_P)   , 
    KC_A        , KC_S   , KC_D   , KC_F           , KC_G             ,                                KC_H               , KC_J   , KC_K   , KC_L   , LT(KL_SMB,KC_SCLN)  , 
    LSFT_T(KC_Z), KC_X   , KC_C   , KC_V           , KC_B             ,                                KC_N               , KC_M   , KC_COMM, KC_DOT , RSFT_T(KC_SLSH)     , 
    KC_LCTL     , KC_LALT, KC_LGUI, LSFT_T(KC_LNG2), LT(KL_SMB,KC_SPC), LT(KL_EMACS,KC_LNG1), KC_BSPC, LT(KL_NUMFN,KC_ENT), _______, _______, _______, KC_RSFT    
  ),
  // 記号専用レイヤー
  [KL_SMB] = LAYOUT_universal(
    S(KC_1)   , KC_LBRC, S(KC_3)   , S(KC_4)   , S(KC_8)   ,                    S(KC_9)   , S(KC_6)   , S(KC_QUOT), S(KC_7)   , S(KC_2)   ,
    S(KC_LBRC), KC_EQL , S(KC_INT3), S(KC_SCLN), KC_RBRC   ,                    KC_NUHS   , KC_MINS   , S(KC_EQL) , S(KC_MINS), KC_QUOT   ,
    XXXXXXX   , XXXXXXX, S(KC_5)   , KC_INT1   , S(KC_RBRC),                    S(KC_NUHS), S(KC_INT1), S(KC_COMM), S(KC_DOT) , S(KC_SLSH),
    _______   , _______, _______   , KC_LSFT   , _______   , _______,  _______, _______   , _______   , _______   , _______   , _______   
  ),
  // FN (TOP)・テンキー(左)・記号レイヤー(右)レイヤー。最左列はスクロール方向を変更。
  [KL_NUMFN] = LAYOUT_universal(
    S(KC_QUOT), KC_7, KC_8  , KC_9   , _______,                       _______, KC_F7  , KC_F8  , KC_F9  , KC_F12 , 
    S(KC_SCLN), KC_4, KC_5  , KC_6   , _______,                       _______, KC_F4  , KC_F5  , KC_F6  , KC_F11 , 
    S(KC_MINS), KC_1, KC_2  , KC_3   , _______,                       _______, KC_F1  , KC_F2  , KC_F3  , KC_F10 , 
     _______  , KC_0, KC_DOT, KC_LSFT, _______, _______,     _______, _______, _______, _______, _______, _______ 
  ),
  // Emacs レイヤー (C-)
  [KL_EMACS] = LAYOUT_universal(
    G(S(KC_F23)), C(KC_X)   , KC_END    , C(KC_R), C(KC_T),                   C(KC_V) , C(KC_Z), KC_TAB    , C(KC_O)  , KC_UP     , 
    KC_HOME     , C(KC_F)   , KC_DEL    , KC_RGHT, ABORT  ,                   KC_BSPC , KC_ENT , CUT_LINE  , C(KC_L)  , C(KC_SCLN),
    G(KC_DOWN)  , OSL(KL_CX), C(KC_C)   , KC_PGDN, KC_LEFT,                   KC_DOWN , KC_ENT , C(KC_COMM), C(KC_DOT), C(KC_Z)   ,
    _______     , C(KC_LALT), C(KC_LGUI), _______, RGB_TOG, _______, KC_PSCR, SET_MARK, _______, _______   , _______  , C(KC_RSFT)
  ),
  // Emacs レイヤー (C-x)
  [KL_CX] = LAYOUT_universal(
    _______, S(C(KC_S)), _______ , _______, _______  ,                               _______, C(KC_Z), _______, A(KC_TAB), _______, 
    _______, C(KC_S)   , G(KC_E) , C(KC_O), _______  ,                               C(KC_A), _______, C(KC_W), _______  , _______, 
    _______, _______   , A(KC_F4), _______, G(KC_TAB),                               _______, _______, _______, _______  , _______, 
    _______, _______   , _______ , _______, _______  , _______,             _______, _______, _______, _______, _______  , _______  
  ),
};

// clang-format on

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t highest_layer = get_highest_layer(state);

    if (highest_layer == KL_SMB) {
        // 垂直スクロール
        keyball_set_scroll_mode(true);
        keyball_set_scrollsnap_mode(KEYBALL_SCROLLSNAP_MODE_VERTICAL);
    } else if (highest_layer == KL_EMACS) {
        // 水平スクロール
        keyball_set_scroll_mode(true);
        keyball_set_scrollsnap_mode(KEYBALL_SCROLLSNAP_MODE_HORIZONTAL);
    } else {
        // 他のレイヤーではスクロールを無効化
        keyball_set_scroll_mode(false);
    }

    // see https://github.com/qmk/qmk_firmware/blob/9c965bb62ec9ea84e68e0a1559dfbc35429df090/quantum/color.h#L49
    // see https://docs.qmk.fm/features/rgblight
    if (rgblight_is_enabled()) {
        switch (highest_layer) {
            case 0:
                rgblight_sethsv_noeeprom(HSV_WHITE);
                break;
            case 1:
                rgblight_sethsv_noeeprom(HSV_AZURE);
                break;
            case 2:
                rgblight_sethsv_noeeprom(HSV_BLUE);
                break;
            case 3:
                rgblight_sethsv_noeeprom(HSV_PURPLE);
                break;
            case 4:
                rgblight_sethsv_noeeprom(HSV_MAGENTA);
                break;
        }
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
