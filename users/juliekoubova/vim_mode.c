/* Copyright 2023 (c) Julie Koubova (julie@koubova.net)
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

#include "vim_mode.h"
#include "print.h"
#include <stdbool.h>

#ifdef VIM_DEBUG
#    define VIM_DPRINT(s) dprint("[vim] " s)
#    define VIM_DPRINTF(...) dprintf("[vim] " __VA_ARGS__)
#else
#    define VIM_DPRINT(s) ((void)0)
#    define VIM_DPRINTF(...) ((void)0)
#endif

typedef enum {
    VIM_SEND_TAP,
    VIM_SEND_PRESS,
    VIM_SEND_RELEASE,
} vim_send_type_t;

typedef enum { VIM_KEY_NONE, VIM_KEY_TAP, VIM_KEY_HELD } vim_key_state_t;

typedef struct {
    bool         append : 1;
    bool         repeating : 1;
    vim_action_t action;
#ifdef VIM_DEBUG
    const char *debug;
#endif
} vim_statemachine_t;

#define VIM_STATEMACHINE_FIRST KC_A
#define VIM_STATEMACHINE_LAST KC_ESC
#define VIM_STATEMACHINE_SIZE (VIM_STATEMACHINE_LAST - VIM_STATEMACHINE_FIRST + 1)
#define VIM_STATEMACHINE_INDEX(key) ((key) >= VIM_STATEMACHINE_FIRST && (key) <= VIM_STATEMACHINE_LAST ? (key - VIM_STATEMACHINE_FIRST) : -1)

#ifdef VIM_DEBUG
#    define VIM_ACTION_DEBUG(str) .debug = str
#else
#    define VIM_ACTION_DEBUG(str)
#endif

#define VIM_ACTION(key, a) [VIM_STATEMACHINE_INDEX(key)] = {.action = (a), VIM_ACTION_DEBUG(#key " => " #a)}
#define VIM_ACTION_REPEATING(key, a) [VIM_STATEMACHINE_INDEX(key)] = {.action = (a), .repeating = true, VIM_ACTION_DEBUG(#key " => " #a " (repeating)")}
#define VIM_BUFFER_THEN_ACTION(key, a) [VIM_STATEMACHINE_INDEX(key)] = {.action = (a), .append = true, VIM_ACTION_DEBUG(#key " => " #a " (buffer)")}

static const vim_statemachine_t vim_statemachine_command[VIM_STATEMACHINE_SIZE] = {
    // clang-format off
    VIM_ACTION(KC_A, VIM_ACTION_RIGHT | VIM_MOD_INSERT_AFTER),
    VIM_ACTION_REPEATING(KC_B, VIM_ACTION_WORD_START),
    VIM_BUFFER_THEN_ACTION(KC_C, VIM_ACTION_LINE | VIM_MOD_CHANGE),
    VIM_BUFFER_THEN_ACTION(KC_D, VIM_ACTION_LINE | VIM_MOD_DELETE),
    VIM_ACTION_REPEATING(KC_E, VIM_ACTION_WORD_END),
    VIM_BUFFER_THEN_ACTION(KC_G, VIM_ACTION_DOCUMENT_START),
    VIM_ACTION_REPEATING(KC_H, VIM_ACTION_LEFT),
    VIM_ACTION(KC_I, VIM_MOD_INSERT_AFTER),
    VIM_ACTION_REPEATING(KC_J, VIM_ACTION_DOWN),
    VIM_ACTION_REPEATING(KC_K, VIM_ACTION_UP),
    VIM_ACTION_REPEATING(KC_L, VIM_ACTION_RIGHT),
    VIM_ACTION_REPEATING(KC_P, VIM_ACTION_PASTE),
    VIM_ACTION(KC_S, VIM_ACTION_RIGHT | VIM_MOD_CHANGE),
    VIM_ACTION_REPEATING(KC_U, VIM_ACTION_UNDO),
    VIM_ACTION(KC_V, VIM_ACTION_VISUAL_MODE),
    VIM_ACTION_REPEATING(KC_W, VIM_ACTION_WORD_END),
    VIM_ACTION_REPEATING(KC_X, VIM_ACTION_RIGHT | VIM_MOD_DELETE),
    VIM_BUFFER_THEN_ACTION(KC_Y, VIM_ACTION_LINE | VIM_MOD_YANK),
    VIM_ACTION(KC_0, VIM_ACTION_LINE_START)
    // clang-format on
};

static const vim_statemachine_t vim_statemachine_command_shift[VIM_STATEMACHINE_SIZE] = {
    // clang-format off
    VIM_ACTION(KC_A, VIM_ACTION_LINE_END | VIM_MOD_INSERT_AFTER),
    VIM_ACTION_REPEATING(KC_B, VIM_ACTION_WORD_START),
    VIM_ACTION(KC_C, VIM_ACTION_LINE_END | VIM_MOD_CHANGE),
    VIM_ACTION(KC_D, VIM_ACTION_LINE_END | VIM_MOD_DELETE),
    VIM_ACTION_REPEATING(KC_E, VIM_ACTION_WORD_END),
    VIM_ACTION_REPEATING(KC_G, VIM_ACTION_DOCUMENT_END),
    VIM_ACTION(KC_I, VIM_ACTION_LINE_START | VIM_MOD_INSERT_AFTER),
    VIM_ACTION_REPEATING(KC_P, VIM_ACTION_PASTE),
    VIM_ACTION(KC_S, VIM_ACTION_LINE | VIM_MOD_CHANGE),
    VIM_ACTION(KC_V, VIM_ACTION_LINE | VIM_MOD_SELECT | VIM_MOD_VISUAL_AFTER),
    VIM_ACTION_REPEATING(KC_W, VIM_ACTION_WORD_END),
    VIM_ACTION_REPEATING(KC_X, VIM_ACTION_LEFT | VIM_MOD_DELETE),
    VIM_ACTION(KC_Y, VIM_ACTION_LINE | VIM_MOD_YANK),
    VIM_ACTION_REPEATING(KC_4, VIM_ACTION_LINE_END),
    VIM_ACTION_REPEATING(KC_6, VIM_ACTION_LINE_START),
    // clang-format on
};

static const vim_statemachine_t vim_statemachine_command_ctrl[VIM_STATEMACHINE_SIZE] = {
    VIM_ACTION_REPEATING(KC_B, VIM_ACTION_PAGE_UP),
    VIM_ACTION_REPEATING(KC_F, VIM_ACTION_PAGE_DOWN),
};

static const vim_statemachine_t vim_statemachine_visual[VIM_STATEMACHINE_SIZE] = {
    // clang-format off
    VIM_ACTION_REPEATING(KC_B, VIM_ACTION_WORD_START | VIM_MOD_SELECT),
    VIM_ACTION(KC_C, VIM_MOD_CHANGE),
    VIM_ACTION(KC_D, VIM_MOD_DELETE),
    VIM_ACTION_REPEATING(KC_E, VIM_ACTION_WORD_END | VIM_MOD_SELECT),
    VIM_BUFFER_THEN_ACTION(KC_G, VIM_ACTION_DOCUMENT_START | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_H, VIM_ACTION_LEFT | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_J, VIM_ACTION_DOWN | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_K, VIM_ACTION_UP | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_L, VIM_ACTION_RIGHT | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_P, VIM_ACTION_PASTE),
    VIM_ACTION(KC_S, VIM_MOD_CHANGE),
    VIM_ACTION(KC_V, VIM_ACTION_COMMAND_MODE),
    VIM_ACTION_REPEATING(KC_W, VIM_ACTION_WORD_END | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_X, VIM_MOD_DELETE),
    VIM_ACTION(KC_Y, VIM_MOD_YANK),
    VIM_ACTION(KC_0, VIM_ACTION_LINE_START | VIM_MOD_SELECT),
    VIM_ACTION(KC_ESCAPE, VIM_ACTION_COMMAND_MODE),
    // clang-format on
};

static const vim_statemachine_t vim_statemachine_visual_shift[VIM_STATEMACHINE_SIZE] = {
    // clang-format off
    VIM_ACTION(KC_C, VIM_ACTION_LINE | VIM_MOD_CHANGE),
    VIM_ACTION(KC_D, VIM_ACTION_LINE | VIM_MOD_DELETE),
    VIM_ACTION(KC_V, VIM_ACTION_LINE | VIM_MOD_SELECT),
    VIM_ACTION(KC_X, VIM_ACTION_LINE | VIM_MOD_DELETE),
    VIM_ACTION(KC_Y, VIM_ACTION_LINE | VIM_MOD_YANK),
    // clang-format on
};

#ifndef VIM_COMMAND_BUFFER_SIZE
#    define VIM_COMMAND_BUFFER_SIZE 10
#endif

static vim_mode_t      vim_mode      = VIM_MODE_INSERT;
static vim_key_state_t vim_key_state = VIM_KEY_NONE;
static uint8_t         vim_mods      = 0; // we modify the actual mods so we can't rely on them

static uint8_t vim_command_buffer[VIM_COMMAND_BUFFER_SIZE] = {};
static uint8_t vim_command_buffer_size                     = 0;

__attribute__((weak)) void vim_mode_changed(vim_mode_t mode) {}

void vim_append_command(uint8_t keycode) {
    if (vim_command_buffer_size == VIM_COMMAND_BUFFER_SIZE) {
        VIM_DPRINT("[vim] Ran out of command buffer space.\n");
        vim_command_buffer_size = 0;
        return;
    }
    vim_command_buffer[vim_command_buffer_size] = keycode;
    vim_command_buffer_size++;

    VIM_DPRINTF("[vim] command buffer: ");
    for (uint8_t i = 0; i < vim_command_buffer_size; i++) {
        VIM_DPRINTF("%d ", vim_command_buffer[i]);
    }
    VIM_DPRINTF("\n");
}

void vim_clear_command(void) {
    VIM_DPRINTF("Cleared command buffer.\n");
    vim_command_buffer_size = 0;
}

uint8_t vim_command_buffer_tail(void) {
    if (vim_command_buffer_size == 0) {
        return KC_NO;
    } else {
        return vim_command_buffer[vim_command_buffer_size - 1];
    }
}

void vim_dprintf_key(uint16_t keycode, keyrecord_t *record, const vim_statemachine_t *state) {
#ifdef VIM_DEBUG
    VIM_DPRINTF("vim_mode=%d vim_mods=%x pressed=%d %s\n", vim_mode, vim_mods, record->event.pressed, state ? state->debug : "NULL");
#else
    VIM_DPRINTF("vim_mode=%d vim_mods=%x pressed=%d keycode=%x\n", vim_mode, vim_mods, record->event.pressed, keycode);
#endif
}

void vim_send(uint8_t mods, uint16_t keycode, vim_send_type_t type) {
    if (mods && (type == VIM_SEND_PRESS || type == VIM_SEND_TAP)) {
        VIM_DPRINTF("register mods=%x\n", mods);
        register_mods(mods);
    }

    switch (type) {
        case VIM_SEND_TAP:
            VIM_DPRINTF("tap keycode=%x\n", keycode);
            tap_code(keycode);
            break;
        case VIM_SEND_PRESS:
            VIM_DPRINTF("register keycode=%x\n", keycode);
            register_code(keycode);
            break;
        case VIM_SEND_RELEASE:
            VIM_DPRINTF("unregister keycode=%x\n", keycode);
            unregister_code(keycode);
            break;
    }

    if (mods && (type == VIM_SEND_RELEASE || type == VIM_SEND_TAP)) {
        VIM_DPRINTF("unregister mods=%x\n", mods);
        unregister_mods(mods);
    }
}

void vim_cancel_os_selection(void) {
    vim_send(0, KC_LEFT, VIM_SEND_TAP);
}

void vim_enter_insert_mode(void) {
    if (vim_mode == VIM_MODE_INSERT) {
        return;
    }
    VIM_DPRINT("Entering insert mode\n");
    vim_mode = VIM_MODE_INSERT;
    vim_mods = 0;
    vim_clear_command();
    clear_keyboard_but_mods();
    vim_mode_changed(vim_mode);
}

void vim_enter_command_mode(void) {
    if (vim_mode == VIM_MODE_COMMAND) {
        return;
    }

    VIM_DPRINTF("Entering command mode vim_mods=%x\n", vim_mods);

    if (vim_mode == VIM_MODE_VISUAL) {
        vim_cancel_os_selection();
    }
    vim_mode = VIM_MODE_COMMAND;
    vim_mods = get_mods();
    vim_clear_command();
    clear_keyboard_but_mods();
    vim_mode_changed(vim_mode);
}

void vim_toggle_command_mode(void) {
    if (vim_mode == VIM_MODE_INSERT) {
        vim_enter_command_mode();
    } else {
        vim_enter_insert_mode();
    }
}

void vim_enter_visual_mode(void) {
    if (vim_mode == VIM_MODE_VISUAL) {
        return;
    }
    VIM_DPRINT("Entering visual mode\n");
    vim_mode      = VIM_MODE_VISUAL;
    vim_key_state = VIM_KEY_NONE; // don't return to insert after vim key is
                                  // released
    vim_clear_command();
    clear_keyboard_but_mods();
    vim_mode_changed(vim_mode);
}

void vim_perform_action(vim_action_t action, vim_send_type_t type) {
    switch (action & VIM_MASK_ACTION) {
        case VIM_ACTION_PASTE:
            vim_send(MOD_LCTL, KC_V, type);
            return;
        case VIM_ACTION_UNDO:
            vim_send(MOD_LCTL, KC_Z, type);
            return;
        case VIM_ACTION_COMMAND_MODE:
            vim_enter_command_mode();
            return;
        case VIM_ACTION_VISUAL_MODE:
            vim_enter_visual_mode();
            return;
        case VIM_ACTION_LINE:
            vim_send(0, KC_END, VIM_SEND_TAP);
            type   = VIM_SEND_TAP;
            action = (action & VIM_MASK_MOD) | VIM_ACTION_LINE_START;
            break;
        default:
            break;
    }

    uint16_t keycode = KC_NO;
    uint8_t  mods    = 0;

    if (action == (VIM_MOD_DELETE | VIM_ACTION_LEFT)) {
        keycode = KC_BSPC;
        action &= ~VIM_MOD_DELETE;
    } else if (action == (VIM_MOD_DELETE | VIM_ACTION_RIGHT)) {
        keycode = KC_DEL;
        action &= ~VIM_MOD_DELETE;
    } else {
        switch (action & VIM_MASK_ACTION) {
            case VIM_ACTION_LEFT:
                keycode = KC_LEFT;
                break;
            case VIM_ACTION_DOWN:
                keycode = KC_DOWN;
                break;
            case VIM_ACTION_UP:
                keycode = KC_UP;
                break;
            case VIM_ACTION_RIGHT:
                keycode = KC_RIGHT;
                break;
            case VIM_ACTION_LINE_START:
                keycode = KC_HOME;
                break;
            case VIM_ACTION_LINE_END:
                keycode = KC_END;
                break;
            case VIM_ACTION_WORD_START:
                keycode = KC_LEFT;
                mods    = MOD_LCTL;
                break;
            case VIM_ACTION_WORD_END:
                keycode = KC_RIGHT;
                mods    = MOD_LCTL;
                break;
            case VIM_ACTION_DOCUMENT_START:
                keycode = KC_HOME;
                mods    = MOD_LCTL;
                break;
            case VIM_ACTION_DOCUMENT_END:
                keycode = KC_END;
                mods    = MOD_LCTL;
                break;
            case VIM_ACTION_PAGE_UP:
                keycode = KC_PAGE_UP;
                break;
            case VIM_ACTION_PAGE_DOWN:
                keycode = KC_PAGE_DOWN;
                break;
            default:
                return;
        }
    }

    switch (vim_command_buffer_tail()) {
        case KC_C:
            action |= VIM_MOD_CHANGE;
            type = VIM_SEND_TAP;
            break;
        case KC_D:
            action |= VIM_MOD_DELETE;
            type = VIM_SEND_TAP;
            break;
        case KC_Y:
            action |= VIM_MOD_YANK;
            type = VIM_SEND_TAP;
            break;
        default:
            break;
    }

    vim_clear_command();

    if (action & (VIM_MOD_DELETE | VIM_MOD_CHANGE)) {
        if (keycode != KC_NO) {
            // keycode is KC_NO in visual mode, where the object is the visual selection
            // send shifted action as a tap
            vim_send(mods | MOD_LSFT, keycode, VIM_SEND_TAP);
        }
        vim_send(MOD_LCTL, KC_X, VIM_SEND_TAP);
        if (action & VIM_MOD_CHANGE) {
            vim_enter_insert_mode();
            return;
        }
    } else if (action & VIM_MOD_YANK) {
        vim_send(mods | MOD_LSFT, keycode, VIM_SEND_TAP);
        vim_send(MOD_LCTL, KC_C, VIM_SEND_TAP);
    } else if (action & VIM_MOD_SELECT) {
        vim_send(mods | MOD_LSFT, keycode, type);
    } else {
        vim_send(mods, keycode, type);
    }

    if (action & VIM_MOD_INSERT_AFTER) {
        vim_enter_insert_mode();
    } else if (action & VIM_MOD_VISUAL_AFTER) {
        vim_enter_visual_mode();
    }
}

const vim_statemachine_t *vim_lookup_statemachine(uint16_t keycode) {
    if (keycode < VIM_STATEMACHINE_FIRST || keycode > VIM_STATEMACHINE_LAST) {
        return NULL;
    }

    uint8_t index = VIM_STATEMACHINE_INDEX(keycode);
    bool    ctrl  = (vim_mods & MOD_BIT(KC_LCTL)) || (vim_mods & MOD_BIT(KC_RCTL));
    bool    shift = (vim_mods & MOD_BIT(KC_LSFT)) || (vim_mods & MOD_BIT(KC_RSFT));

    if (vim_mode == VIM_MODE_COMMAND) {
        if (ctrl) {
            return &vim_statemachine_command_ctrl[index];
        } else if (shift) {
            return &vim_statemachine_command_shift[index];
        } else if (vim_mods == 0) {
            return &vim_statemachine_command[index];
        }
    } else if (vim_mode == VIM_MODE_VISUAL) {
        if (vim_mods == 0) {
            return &vim_statemachine_visual[index];
        } else if (shift) {
            return &vim_statemachine_visual_shift[index];
        }
    }

    return NULL;
}

void vim_process_command(uint16_t keycode, keyrecord_t *record) {
    const vim_statemachine_t *state = vim_lookup_statemachine(keycode);
    vim_dprintf_key(keycode, record, state);
    if (state == NULL) {
        VIM_DPRINT("Not a statemachine key.");
        return;
    }

    if (record->event.pressed) {
        if (state->append && state->action && vim_command_buffer_tail() == keycode) {
            vim_perform_action(state->action, VIM_SEND_TAP);
        } else if (state->append) {
            vim_append_command(keycode);
        } else if (state->repeating) {
            vim_perform_action(state->action, VIM_SEND_PRESS);
        } else {
            vim_perform_action(state->action, VIM_SEND_TAP);
        }
    } else if (state->repeating) {
        vim_perform_action(state->action, VIM_SEND_RELEASE);
    }
}

void vim_set_mod(uint16_t keycode, bool pressed) {
    uint8_t bit = MOD_BIT(keycode);
    vim_mods    = pressed ? (vim_mods | bit) : (vim_mods & ~bit);
    VIM_DPRINTF("vim_mods = %x\n", vim_mods);
}

void vim_process_vim_key(bool pressed) {
    if (pressed) {
        if (vim_mode == VIM_MODE_INSERT) {
            VIM_DPRINT("Vim key pressed in insert mode\n");
            vim_key_state = VIM_KEY_TAP;
            vim_enter_command_mode();
        } else {
            VIM_DPRINT("Vim key pressed in non-insert mode\n");
            vim_key_state = VIM_KEY_NONE;
            vim_enter_insert_mode();
        }
    } else {
        VIM_DPRINTF("Vim key released, vim_key_state=%d\n", vim_key_state);
        vim_key_state_t prev_vim_key_state = vim_key_state;
        vim_key_state                      = VIM_KEY_NONE;

        switch (prev_vim_key_state) {
            case VIM_KEY_NONE:
                // set when visual mode is entered from a vi key tap.
                // in that case, we want to stay in the selected mode
                break;
            case VIM_KEY_TAP:
                vim_enter_command_mode();
                break;
            case VIM_KEY_HELD:
                vim_enter_insert_mode();
                break;
        }
    }
}

bool process_record_vim(uint16_t keycode, keyrecord_t *record, uint16_t vim_keycode) {
    if (keycode == vim_keycode) {
        vim_process_vim_key(record->event.pressed);
        return false;
    }

    if (vim_mode != VIM_MODE_INSERT) {
        if (IS_MODIFIER_KEYCODE(keycode)) {
            vim_set_mod(keycode, record->event.pressed);
            return false;
        }

        if (record->event.pressed) {
            vim_key_state = VIM_KEY_HELD;
        }

        VIM_DPRINTF("vim_key_state=%d\n", vim_key_state);
        vim_process_command(keycode, record);
        return false;
    }

    return true;
}

bool vim_is_active_key(uint16_t keycode) {
    if (vim_mode == VIM_MODE_INSERT) {
        return false;
    }

    const vim_statemachine_t *state = vim_lookup_statemachine(keycode);
    return state != NULL && state->action != VIM_ACTION_NONE;
}

vim_mode_t vim_get_mode(void) {
    return vim_mode;
}
