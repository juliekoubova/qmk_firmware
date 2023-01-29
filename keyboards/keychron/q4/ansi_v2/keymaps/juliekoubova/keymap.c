/* Copyright 2022 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H


enum layers {
    BASE,
    FN,
};

enum key_codes {
    QK_VIM_B = SAFE_RANGE,
    QK_VIM_F,
};

enum tap_dances {
    TD_VIM_G,
};

// clang-format off

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [BASE] = LAYOUT_ansi_61(
        KC_ESC,      KC_1,     KC_2,        KC_3,    KC_4,     KC_5,         KC_6,       KC_7,    KC_8,    KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,
        KC_TAB,      KC_Q,     KC_W,        KC_E,    KC_R,     KC_T,         KC_Y,       KC_U,    KC_I,    KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,
        TT(FN),      KC_A,     KC_S,        KC_D,    KC_F,     KC_G,         KC_H,       KC_J,    KC_K,    KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,
        KC_LSFT,     KC_Z,     KC_X,        KC_C,    KC_V,     KC_B,         KC_N,       KC_M,    KC_COMM, KC_DOT,   KC_SLSH,                      KC_RSFT,
        KC_LCTL,     KC_LOPT,  KC_LCMD,                                      KC_SPC,                                 TT(FN),   KC_RCMD,  KC_ROPT,  KC_RCTL),

    [FN] = LAYOUT_ansi_61(
        KC_TILD,     KC_F1,    KC_F2,       KC_F3,   KC_F4,    KC_F5,        KC_F6,      KC_F7,   KC_F8,   KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_LEFT,
        XXXXXXX,     XXXXXXX,  C(KC_RIGHT), XXXXXXX, KC_INS,   XXXXXXX,      XXXXXXX,    XXXXXXX, KC_HOME, XXXXXXX,  XXXXXXX,  KC_VOLD,  KC_VOLU,  KC_MUTE,
        _______,     KC_END,   XXXXXXX,     XXXXXXX, QK_VIM_F, TD(TD_VIM_G), KC_LEFT,    KC_DOWN, KC_UP,   KC_RIGHT, XXXXXXX,  XXXXXXX,            XXXXXXX,
        _______,               XXXXXXX,     KC_DEL,  XXXXXXX,  XXXXXXX,      QK_VIM_B,   XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX,  XXXXXXX,            _______,
        _______,     _______,  _______,                                      KC_RIGHT,                               _______,  _______,  _______,  _______)
};

// clang-format on

// ============================================================================
// Key Overrides
// ============================================================================

#define LAYER_BIT(layer) (1 << layer)

// Fn+X       = forward delete
// Fn+Shift+X = backspace
const key_override_t fn_shift_x = ko_make_with_layers(
    MOD_MASK_SHIFT, KC_DEL,
    KC_BSPC,
    LAYER_BIT(FN)
);

// Fn+B = Ctrl-left (jump one word to the left)
const key_override_t fn_b = ko_make_with_layers_and_negmods(
    0, QK_VIM_B,
    C(KC_LEFT),
    LAYER_BIT(FN),
    MOD_MASK_CTRL
);

// Fn+Ctrl+B = Page Up
const key_override_t fn_ctrl_b = ko_make_with_layers(
    MOD_MASK_CTRL, QK_VIM_B,
    KC_PAGE_UP,
    LAYER_BIT(FN)
);

// Fn+Ctrl+F = Page Down
const key_override_t fn_ctrl_f = ko_make_with_layers(
    MOD_MASK_CTRL, QK_VIM_F,
    KC_PAGE_DOWN,
    LAYER_BIT(FN)
);


const key_override_t **key_overrides = (const key_override_t*[]){
    &fn_shift_x,
    &fn_b,
    &fn_ctrl_b,
    &fn_ctrl_f,
    NULL
};

// ============================================================================
// Tap Dances
// ============================================================================

qk_tap_dance_action_t tap_dance_actions[] = {
    [TD_VIM_G] = ACTION_TAP_DANCE_DOUBLE(C(KC_END), C(KC_HOME)),
};

// ============================================================================
// RGB Matrix
// ============================================================================

#ifdef RGB_MATRIX_ENABLE

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    uint8_t layer = get_highest_layer(layer_state);

    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            uint8_t index = g_led_config.matrix_co[row][col];
            if (index < led_min || index >= led_max || index == NO_LED) {\
                continue;
            }

            if (get_highest_layer(layer_state) > 0) {
                uint16_t keycode = keymap_key_to_keycode(layer, (keypos_t){col,row});
                if (keycode > KC_TRNS) {
                    rgb_matrix_set_color(index, RGB_RED);
                    continue;
                }
            };

            uint16_t keycode = keymap_key_to_keycode(BASE, (keypos_t){col,row});
            if (keycode == KC_LSFT) {
                if (is_caps_word_on() || host_keyboard_led_state().caps_lock) {
                    rgb_matrix_set_color(index, RGB_TURQUOISE);
                    continue;
                }
            }

            rgb_matrix_set_color(index, RGB_OFF);
        }
    }
    return false;
}

#endif