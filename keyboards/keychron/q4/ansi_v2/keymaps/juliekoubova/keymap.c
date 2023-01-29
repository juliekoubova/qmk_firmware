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
    QK_VIM = SAFE_RANGE,
    QK_VIM_B,
    QK_VIM_F,
};

enum tap_dances {
    TD_FN_ESC,
    TD_VIM_G,
};

// clang-format off

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [BASE] = LAYOUT_ansi_61(
        KC_ESC,        KC_1,     KC_2,        KC_3,    KC_4,     KC_5,         KC_6,       KC_7,    KC_8,    KC_9,     KC_0,           KC_MINS,  KC_EQL,   KC_BSPC,
        KC_TAB,        KC_Q,     KC_W,        KC_E,    KC_R,     KC_T,         KC_Y,       KC_U,    KC_I,    KC_O,     KC_P,           KC_LBRC,  KC_RBRC,  KC_BSLS,
        TD(TD_FN_ESC), KC_A,     KC_S,        KC_D,    KC_F,     KC_G,         KC_H,       KC_J,    KC_K,    KC_L,     KC_SCLN,        KC_QUOT,            KC_ENT,
        KC_LSFT,       KC_Z,     KC_X,        KC_C,    KC_V,     KC_B,         KC_N,       KC_M,    KC_COMM, KC_DOT,   KC_SLSH,                            KC_RSFT,
        KC_LCTL,       KC_LOPT,  KC_LCMD,                                      KC_SPC,                                 QK_VIM,  KC_RCMD,  KC_ROPT,  KC_RCTL),

    [FN] = LAYOUT_ansi_61(
        KC_GRAVE,      KC_F1,    KC_F2,       KC_F3,   KC_F4,    KC_F5,        KC_F6,      KC_F7,   KC_F8,   KC_F9,    KC_F10,         KC_F11,   KC_F12,   KC_LEFT,
        XXXXXXX,       XXXXXXX,  C(KC_RIGHT), XXXXXXX, KC_INS,   XXXXXXX,      XXXXXXX,    XXXXXXX, KC_HOME, XXXXXXX,  XXXXXXX,        KC_VOLD,  KC_VOLU,  KC_MUTE,
        _______,       KC_END,   XXXXXXX,     XXXXXXX, QK_VIM_F, TD(TD_VIM_G), KC_LEFT,    KC_DOWN, KC_UP,   KC_RIGHT, XXXXXXX,        XXXXXXX,            XXXXXXX,
        _______,       XXXXXXX,  KC_DEL,      XXXXXXX, XXXXXXX,  QK_VIM_B,     XXXXXXX,    XXXXXXX, XXXXXXX, XXXXXXX,  XXXXXXX,                            _______,
        _______,       _______,  _______,                                      KC_RIGHT,                               _______,        _______,  _______,  _______)
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

typedef enum {
    TD_NONE,
    TD_SINGLE_TAP,
    TD_DOUBLE_TAP,
    TD_SINGLE_HOLD,
} td_state_t;

static td_state_t td_state;

td_state_t cur_dance(qk_tap_dance_state_t *state) {
    switch (state->count) {
        case 1: return state->pressed ? TD_SINGLE_HOLD : TD_SINGLE_TAP;
        case 2: return TD_DOUBLE_TAP;
        default: return TD_NONE;
    }
}

void fn_esc_finished(qk_tap_dance_state_t *state, void *user_data) {
    td_state = cur_dance(state);
    switch (td_state) {
        case TD_SINGLE_TAP:
            tap_code(KC_ESC);
            break;
        case TD_SINGLE_HOLD:
            layer_on(FN);
            break;
        case TD_DOUBLE_TAP:
            if (layer_state_is(FN)) {
                layer_off(FN);
            } else {
                layer_on(FN);
            }
            break;
        default: break;
    }
}

void fn_esc_reset(qk_tap_dance_state_t *state, void *user_data) {
    if (td_state == TD_SINGLE_HOLD) {
        layer_off(FN);
    }
    td_state = TD_NONE;
}

qk_tap_dance_action_t tap_dance_actions[] = {
    [TD_FN_ESC] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, fn_esc_finished, fn_esc_reset),
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

#include "print.h"

#define VIM_DPRINT(s)    dprint("[vim] " s)
#define VIM_DPRINTF(...) dprintf("[vim] " __VA_ARGS__)

typedef enum {
    VIM_INSERT_MODE,
    VIM_COMMAND_MODE,
    VIM_VISUAL_MODE,
} vim_mode_t;

static vim_mode_t vim_mode = VIM_INSERT_MODE;
static bool vim_key_pressed = false;

void vim_toggle_command_mode(void) {
    if (vim_mode == VIM_INSERT_MODE) {
        VIM_DPRINT("Entering command mode\n");
        vim_mode = VIM_COMMAND_MODE;
    } else {
        VIM_DPRINT("Entering insert mode\n");
        vim_mode = VIM_INSERT_MODE;
    }
}

void process_vim_command(uint16_t keycode, keyrecord_t *record) {
    uint16_t new_keycode = KC_NO;
    switch (keycode) {
        case KC_H: new_keycode = KC_LEFT; break;
        case KC_J: new_keycode = KC_DOWN; break;
        case KC_K: new_keycode = KC_UP; break;
        case KC_L: new_keycode = KC_RIGHT; break;
        default: break;
    }
    if (new_keycode != KC_NO) {
        if (record->event.pressed) {
            VIM_DPRINTF("Pressing %d\n", new_keycode);
            register_code(new_keycode);
        } else {
            VIM_DPRINTF("Releasing %d\n", new_keycode);
            unregister_code(new_keycode);
        }
    }
}

bool process_record_vim(uint16_t keycode, keyrecord_t *record, uint16_t vim_keycode) {
    if (record->event.pressed) {
        static bool tapped = false;
        static uint16_t timer = 0;
        if (keycode == vim_keycode) {
            if (tapped && !timer_expired(record->event.time, timer)) {
                // double-tapped the vim key, toggle command mode
                vim_toggle_command_mode();
                return false;
            }
            VIM_DPRINT("Vim key pressed\n");
            tapped = true;
            timer = record->event.time + GET_TAPPING_TERM(keycode, record);
            vim_key_pressed = true;
            return false;
        } else {
            tapped = false;
        }
    } else if (keycode == vim_keycode) {
        VIM_DPRINT("Vim key released\n");
        vim_key_pressed = false;
        return false;
    }

    if (vim_mode == VIM_COMMAND_MODE || vim_key_pressed) {
        process_vim_command(keycode, record);
        return false;
    }

    return true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    return process_record_vim(keycode, record, QK_VIM);
}

void keyboard_post_init_user(void) {
  debug_enable=true;
}