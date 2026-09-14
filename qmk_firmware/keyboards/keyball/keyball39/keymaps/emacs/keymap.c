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
    ABORT,
    COPY_REGION,
    CUT_WORD,
    CUT_WORD_BACKWARD,
    MARK_WORD
};

/* ------ */
/* Macros */
/* ------ */

bool set_mark_active = false;  // マーク状態を保持
static bool mark_inactive = true;
static matrix_row_t mark_navigation_keys[MATRIX_ROWS];
static uint8_t mark_navigation_count;

// S(kc) and CUT_LINE use weak left Shift; reserve weak right Shift for Mark.
#define MARK_SHIFT MOD_BIT(KC_RSFT)

static void set_mark_mode(bool active) {
    set_mark_active = active;
    mark_inactive = !active;
}

static void restore_mark_shift(void) {
    if (mark_navigation_count) {
        add_weak_mods(MARK_SHIFT);
    }
}

static void end_mark(bool send_report) {
    bool had_navigation = mark_navigation_count != 0;
    set_mark_mode(false);
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
    if (keycode == COPY_REGION || keycode == CUT_WORD || keycode == CUT_WORD_BACKWARD) {
        end_mark(false);
    } else if (keycode == MARK_WORD) {
        set_mark_mode(true);
    }

    uint8_t mods = get_mods();
    uint8_t weak_mods = get_weak_mods();
    clear_mods();
    clear_weak_mods();
    // A pending one-shot is consumed by the macro, not applied to the next key.
    clear_oneshot_mods();
    send_keyboard_report();

    switch (keycode) {
        case CUT_LINE:
        case CUT_WORD:
        case CUT_WORD_BACKWARD:
            tap_code16(keycode == CUT_LINE ? S(KC_END) : keycode == CUT_WORD ? C(S(KC_RIGHT)) : C(S(KC_LEFT)));
            wait_ms(10);
            tap_code16(C(KC_X));
            break;
        case COPY_REGION:
            tap_code16(C(KC_C));
            break;
        case MARK_WORD:
            tap_code16(C(S(KC_RIGHT)));
            break;
        case ABORT:
            tap_code(KC_ESC);
            break;
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
        case COPY_REGION:
        case CUT_WORD:
        case CUT_WORD_BACKWARD:
        case MARK_WORD:
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
                    set_mark_mode(true);
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

// Mutually exclusive rules keep modifier events from replacing a Mark rule with its plain counterpart.
#define EMACS_NAV_OVERRIDE(mods, key, output, condition) { \
    .trigger_mods = (mods),                              \
    .trigger = (key),                                    \
    .replacement = (output),                            \
    .layers = ~0,                                       \
    .negative_mod_mask = MOD_MASK_CG,                    \
    .suppressed_mods = (mods) | ((IS_QK_MODS((uint16_t)(key)) && ((key) & QK_LSFT)) ? MOD_MASK_SHIFT : 0), \
    .options = ALT_OVERRIDE_OPTIONS,                     \
    .enabled = (condition),                             \
}

static bool emacs_macro_action(bool activated, void *context) {
    if (activated) {
        send_edit_macro((uint16_t)(uintptr_t)context);
    }
    return false;
}

// Editing macros fire only on the trigger press, never on a later modifier press.
#define EMACS_MACRO_OVERRIDE(mods, key, output) {                                        \
    .trigger_mods = (mods),                                                             \
    .trigger = (key),                                                                   \
    .replacement = (output),                                                           \
    .layers = ~0,                                                                      \
    .negative_mod_mask = MOD_MASK_CG,                                                   \
    .suppressed_mods = (mods),                                                          \
    .options = ko_option_activation_trigger_down | ko_option_no_reregister_trigger,     \
    .custom_action = emacs_macro_action,                                                \
    .context = (void *)(uintptr_t)(output),                                             \
}

// Selection Shift uses QMK's override modifiers, even when input Shift is suppressed.
const key_override_t mark_alt_v_to_pageup = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, KC_V, S(KC_PGUP), &set_mark_active);
const key_override_t mark_alt_b_to_ctrl_left = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, KC_B, C(S(KC_LEFT)), &set_mark_active);
const key_override_t mark_alt_f_to_ctrl_right = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, KC_F, C(S(KC_RIGHT)), &set_mark_active);
const key_override_t mark_alt_less_to_home = EMACS_NAV_OVERRIDE(MOD_MASK_ALT | MOD_MASK_SHIFT, KC_COMM, C(S(KC_HOME)), &set_mark_active);
const key_override_t mark_alt_greater_to_end = EMACS_NAV_OVERRIDE(MOD_MASK_ALT | MOD_MASK_SHIFT, KC_DOT, C(S(KC_END)), &set_mark_active);
const key_override_t mark_alt_symbol_less_to_home = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, S(KC_COMM), C(S(KC_HOME)), &set_mark_active);
const key_override_t mark_alt_symbol_greater_to_end = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, S(KC_DOT), C(S(KC_END)), &set_mark_active);

const key_override_t alt_v_to_pageup = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, KC_V, KC_PGUP, &mark_inactive);

// ALT+B -> CTRL+LEFT
const key_override_t alt_b_to_ctrl_left = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, KC_B, C(KC_LEFT), &mark_inactive);
// ALT+F -> CTRL+RIGHT
const key_override_t alt_f_to_ctrl_right = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, KC_F, C(KC_RIGHT), &mark_inactive);
const key_override_t alt_less_to_home = EMACS_NAV_OVERRIDE(MOD_MASK_ALT | MOD_MASK_SHIFT, KC_COMM, C(KC_HOME), &mark_inactive);
const key_override_t alt_greater_to_end = EMACS_NAV_OVERRIDE(MOD_MASK_ALT | MOD_MASK_SHIFT, KC_DOT, C(KC_END), &mark_inactive);
const key_override_t alt_symbol_less_to_home = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, S(KC_COMM), C(KC_HOME), &mark_inactive);
const key_override_t alt_symbol_greater_to_end = EMACS_NAV_OVERRIDE(MOD_MASK_ALT, S(KC_DOT), C(KC_END), &mark_inactive);
// ALT+Y -> Win(GUI)+V
const key_override_t alt_y_to_gui_v = ko_make_with_layers_negmods_and_options(MOD_MASK_ALT, KC_Y, LGUI(KC_V), ~0, MOD_MASK_CG, ALT_OVERRIDE_OPTIONS);

const key_override_t alt_w_to_copy = EMACS_MACRO_OVERRIDE(MOD_MASK_ALT, KC_W, COPY_REGION);
const key_override_t alt_d_to_cut_word = EMACS_MACRO_OVERRIDE(MOD_MASK_ALT, KC_D, CUT_WORD);
const key_override_t alt_backspace_to_cut_word = EMACS_MACRO_OVERRIDE(MOD_MASK_ALT, KC_BSPC, CUT_WORD_BACKWARD);
const key_override_t alt_at_to_mark_word = EMACS_MACRO_OVERRIDE(MOD_MASK_ALT, KC_LBRC, MARK_WORD);

const key_override_t **key_overrides = (const key_override_t *[]){
    &mark_alt_v_to_pageup,
    &mark_alt_b_to_ctrl_left,
    &mark_alt_f_to_ctrl_right,
    &mark_alt_less_to_home,
    &mark_alt_greater_to_end,
    &mark_alt_symbol_less_to_home,
    &mark_alt_symbol_greater_to_end,
    &alt_v_to_pageup,
    &alt_b_to_ctrl_left,
    &alt_f_to_ctrl_right,
    &alt_less_to_home,
    &alt_greater_to_end,
    &alt_symbol_less_to_home,
    &alt_symbol_greater_to_end,
    &alt_y_to_gui_v,
    &alt_w_to_copy,
    &alt_d_to_cut_word,
    &alt_backspace_to_cut_word,
    &alt_at_to_mark_word,
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
