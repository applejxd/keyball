/*
This is the c configuration file for the keymap

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

#pragma once

// レイヤー数指定
// see https://mazcon.hatenablog.com/entry/2023/11/10/080000
#define DYNAMIC_KEYMAP_LAYER_COUNT 5

#define TAP_CODE_DELAY 5

#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define AUTO_MOUSE_DEFAULT_LAYER 2

/* ------- */
/* 容量節約 */
/* ------- */

// see https://zenn.dev/koron/articles/98324ab760e83a

// Cherry MX Lock キーのサポートを無効化
#undef LOCKING_SUPPORT_ENABLE
#undef LOCKING_RESYNC_ENABLE
// use 8bit layer state (max 8 layers)
#define LAYER_STATE_8BIT

// ミュージックモードを無効化
#define NO_MUSIC_MODE
// ワンショットキーを無効化
#undef NO_ACTION_ONESHOT

// #define RGBLIGHT_LAYERS
// #define RGBLIGHT_DEFAULT_MODE RGBLIGHT_MODE_STATIC_LIGHT
// すべてのエフェクトを無効化
#define RGBLIGHT_EFFECT_BREATHING       false
#define RGBLIGHT_EFFECT_RAINBOW_MOOD    false
#define RGBLIGHT_EFFECT_RAINBOW_SWIRL   false
#define RGBLIGHT_EFFECT_SNAKE           false
#define RGBLIGHT_EFFECT_KNIGHT          false
#define RGBLIGHT_EFFECT_CHRISTMAS       false
#define RGBLIGHT_EFFECT_STATIC_GRADIENT false
#define RGBLIGHT_EFFECT_RGB_TEST        false
#define RGBLIGHT_EFFECT_ALTERNATING     false
#define RGBLIGHT_EFFECT_TWINKLE         false
