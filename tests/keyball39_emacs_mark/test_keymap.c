#include "quantum.h"
#include "color.h"
#include "keyball39/keyball39.h"

// Only the physical OLED/RGB/trackball peripherals are outside this test.
bool rgblight_is_enabled(void) {
    return false;
}

void rgblight_sethsv_noeeprom(uint8_t hue, uint8_t saturation, uint8_t value) {}

void keyball_set_scroll_mode(bool mode) {}

void keyball_set_scrollsnap_mode(keyball_scrollsnap_mode_t mode) {}

#define QMK_KEYBOARD_H "keyball39/keyball39.h"
// TestFixture supplies dynamic keymaps; compile the production map separately.
#define keymaps emacs_keymaps
#include "keyball39/keymaps/emacs/keymap.c"
#undef keymaps

const uint8_t emacs_keymap_layer_count = ARRAY_SIZE(emacs_keymaps);
