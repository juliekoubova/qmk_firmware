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

#include "print.h"
#include "vim_mode.h"

#define VIM_DPRINT(s) dprint("[vim] " s)
#define VIM_DPRINTF(...) dprintf("[vim] " __VA_ARGS__)

enum {
    VIM_ACTION_NONE,

    VIM_ACTION_LEFT,
    VIM_ACTION_DOWN,
    VIM_ACTION_UP,
    VIM_ACTION_RIGHT,

    VIM_ACTION_DOCUMENT_START,
    VIM_ACTION_DOCUMENT_END,
    VIM_ACTION_LINE_START,
    VIM_ACTION_LINE_END,
    VIM_ACTION_PAGE_UP,
    VIM_ACTION_PAGE_DOWN,
    VIM_ACTION_WORD_START,
    VIM_ACTION_WORD_END,

    VIM_ACTION_PASTE,
    VIM_ACTION_UNDO,
};

enum {
    VIM_MOD_MOVE   = 0,
    VIM_MOD_CHANGE = 0x0100,
    VIM_MOD_DELETE = 0x0200,
    VIM_MOD_SELECT = 0x0400,
    VIM_MOD_YANK   = 0x0800,
};

enum {
    VIM_MASK_ACTION = 0x00ff,
    VIM_MASK_MOD    = 0xff00,
};

typedef uint16_t vim_action_t;

typedef enum {
    VIM_SEND_TAP,
    VIM_SEND_PRESS,
    VIM_SEND_RELEASE,
} vim_send_type_t;

#ifndef VIM_COMMAND_BUFFER_SIZE
#    define VIM_COMMAND_BUFFER_SIZE 10
#endif

static vim_mode_t vim_mode        = VIM_INSERT_MODE;
static bool       vim_key_pressed = false;

// track modifier state separately. we modify the actual mods so we can't rely on them
static uint8_t vim_mods = 0;

static uint8_t vim_command_buffer[VIM_COMMAND_BUFFER_SIZE] = {};
static uint8_t vim_command_buffer_size                     = 0;

__attribute__((weak)) void vim_mode_changed(vim_mode_t mode) {}

void vim_append_command(uint8_t keycode) {
    if (vim_command_buffer_size == VIM_COMMAND_BUFFER_SIZE) {
        print("[vim] Ran out of command buffer space.\n");
        vim_command_buffer_size = 0;
        return;
    }
    vim_command_buffer[vim_command_buffer_size] = keycode;
    vim_command_buffer_size++;

    dprint("[vim] command buffer: ");
    for (uint8_t i = 0; i < vim_command_buffer_size; i++) {
        dprintf("%d ", vim_command_buffer[i]);
    }
    dprint("\n");
}

void vim_clear_command(void) {
    VIM_DPRINT("Cleared command buffer.\n");
    vim_command_buffer_size = 0;
}

uint8_t vim_command_buffer_tail(void) {
    if (vim_command_buffer_size == 0) {
        return KC_NO;
    } else {
        return vim_command_buffer[vim_command_buffer_size - 1];
    }
}

void vim_dprintf_key(const char *function, uint16_t keycode, keyrecord_t *record) {
    VIM_DPRINTF("%s vim_mods=%x keycode=%x pressed=%d\n", function, vim_mods, keycode, record->event.pressed);
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

void vim_delete_os_selection(void) {
    vim_send(MOD_LCTL, KC_X, VIM_SEND_TAP);
}

void vim_yank_os_selection(void) {
    vim_send(MOD_LCTL, KC_C, VIM_SEND_TAP);
}

void vim_enter_insert_mode(void) {
    if (vim_mode == VIM_INSERT_MODE) {
        return;
    }
    VIM_DPRINT("Entering insert mode\n");
    vim_mode = VIM_INSERT_MODE;
    vim_mods = 0;
    vim_clear_command();
    clear_keyboard();
    vim_mode_changed(vim_mode);
}

void vim_enter_command_mode(void) {
    if (vim_mode == VIM_COMMAND_MODE) {
        return;
    }

    VIM_DPRINTF("Entering command mode vim_mods=%x\n", vim_mods);

    if (vim_mode == VIM_VISUAL_MODE) {
        vim_cancel_os_selection();
    }
    vim_mode = VIM_COMMAND_MODE;
    vim_mods = get_mods();
    vim_clear_command();
    vim_mode_changed(vim_mode);
}

void vim_enter_visual_mode(void) {
    if (vim_mode == VIM_VISUAL_MODE) {
        return;
    }
    VIM_DPRINT("Entering visual mode\n");
    vim_mode = VIM_VISUAL_MODE;
    vim_clear_command();
    vim_mode_changed(vim_mode);
}

void vim_toggle_command_mode(void) {
    if (vim_mode == VIM_INSERT_MODE) {
        vim_enter_command_mode();
    } else {
        vim_enter_insert_mode();
    }
}

void vim_perform_action(vim_action_t action, vim_send_type_t type) {
    // these are repeating, so we send press or release based on passed type
    switch (action & VIM_MASK_ACTION) {
        case VIM_ACTION_PASTE:
            vim_send(MOD_LCTL, KC_V, type);
            return;
        case VIM_ACTION_UNDO:
            vim_send(MOD_LCTL, KC_Z, type);
            return;
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

    if (action & (VIM_MOD_DELETE | VIM_MOD_CHANGE)) {
        // send shifted action as a tap
        vim_send(mods | MOD_LSFT, keycode, VIM_SEND_TAP);
        vim_delete_os_selection();
        if (action & VIM_MOD_CHANGE) {
            vim_enter_insert_mode();
        }
    } else if (action & VIM_MOD_YANK) {
        vim_send(mods | MOD_LSFT, keycode, VIM_SEND_TAP);
        vim_yank_os_selection();
    } else if (action & VIM_MOD_SELECT) {
        vim_send(mods | MOD_LSFT, keycode, type);
    } else {
        vim_send(mods, keycode, type);
    }
}

vim_action_t vim_get_action(uint16_t keycode) {
    if (vim_mods == 0) {
        switch (keycode) {
            case KC_B:
                return VIM_ACTION_WORD_START;
            case KC_E:
            case KC_W:
                return VIM_ACTION_WORD_END;
            case KC_H:
                return VIM_ACTION_LEFT;
            case KC_J:
                return VIM_ACTION_DOWN;
            case KC_K:
                return VIM_ACTION_UP;
            case KC_L:
                return VIM_ACTION_RIGHT;
            case KC_P:
                return VIM_ACTION_PASTE;
            case KC_U:
                return VIM_ACTION_UNDO;
            case KC_X:
                return VIM_ACTION_RIGHT | VIM_MOD_DELETE;
            case KC_0:
                return VIM_ACTION_LINE_START;
            default:
                return VIM_ACTION_NONE;
        }
    } else if (vim_mods & MOD_MASK_SHIFT) {
        switch (keycode) {
            case KC_4: /*$*/
                return VIM_ACTION_LINE_END;
            case KC_6: /*^*/
                return VIM_ACTION_LINE_START;
            case KC_B:
                return VIM_ACTION_WORD_START;
            case KC_E:
            case KC_W:
                return VIM_ACTION_WORD_END;
            case KC_G:
                return VIM_ACTION_DOCUMENT_END;
            case KC_P:
                return VIM_ACTION_PASTE;
            case KC_X:
                return VIM_ACTION_LEFT | VIM_MOD_DELETE;
            default:
                return VIM_ACTION_NONE;
        }
    } else if (vim_mods & MOD_MASK_CTRL) {
        switch (keycode) {
            case KC_B:
                return VIM_ACTION_PAGE_UP;
            case KC_F:
                return VIM_ACTION_PAGE_DOWN;
            default:
                return VIM_ACTION_NONE;
        }
    } else {
        return VIM_ACTION_NONE;
    }
}

void vim_delete_line(void) {
    vim_perform_action(VIM_ACTION_LINE_START, VIM_SEND_TAP);
    vim_perform_action(VIM_MOD_SELECT | VIM_ACTION_LINE_END, VIM_SEND_TAP);
    vim_delete_os_selection();
}

void vim_yank_line(void) {
    vim_perform_action(VIM_ACTION_LINE_START, VIM_SEND_TAP);
    vim_perform_action(VIM_MOD_SELECT | VIM_ACTION_LINE_END, VIM_SEND_TAP);
    vim_yank_os_selection();
    vim_perform_action(VIM_ACTION_LINE_START, VIM_SEND_TAP);
}

void process_vim_command(uint16_t keycode, keyrecord_t *record) {
    vim_dprintf_key(__func__, keycode, record);
    if (record->event.pressed) {
        if (vim_mods == 0) {
            switch (keycode) {
                case KC_1 ... KC_9:
                case KC_C:
                case KC_D:
                case KC_Y:
                    vim_append_command(keycode);
                    return;
                case KC_A:
                    vim_perform_action(VIM_ACTION_RIGHT, VIM_SEND_TAP);
                    vim_enter_insert_mode();
                    return;
                case KC_G:
                    if (vim_command_buffer_tail() == KC_G) {
                        vim_clear_command();
                        vim_perform_action(VIM_ACTION_DOCUMENT_START, VIM_SEND_TAP);
                        return;
                    } else {
                        vim_append_command(KC_G);
                        return;
                    }
                case KC_I:
                    vim_enter_insert_mode();
                    return;
                case KC_V:
                    vim_enter_visual_mode();
                    return;
            }
        } else if (vim_mods & MOD_MASK_SHIFT) {
            switch (keycode) {
                case KC_A:
                    vim_perform_action(VIM_ACTION_LINE_END, VIM_SEND_TAP);
                    vim_enter_insert_mode();
                    return;
                case KC_C:
                    vim_perform_action(VIM_MOD_SELECT | VIM_ACTION_LINE_END, VIM_SEND_TAP);
                    vim_delete_os_selection();
                    vim_enter_insert_mode();
                    return;
                case KC_D:
                    vim_perform_action(VIM_MOD_SELECT | VIM_ACTION_LINE_END, VIM_SEND_TAP);
                    vim_delete_os_selection();
                    return;
                case KC_I:
                    vim_perform_action(VIM_ACTION_LINE_START, VIM_SEND_TAP);
                    vim_enter_insert_mode();
                    return;
                case KC_V:
                    vim_perform_action(VIM_ACTION_LINE_START, VIM_SEND_TAP);
                    vim_perform_action(VIM_ACTION_LINE_END | VIM_MOD_SELECT, VIM_SEND_TAP);
                    vim_enter_visual_mode();
                    return;
                case KC_Y:
                    vim_yank_line();
                    return;
            }
        }
    }

    vim_action_t    action    = vim_get_action(keycode);
    vim_send_type_t send_type = VIM_SEND_TAP;

    switch (vim_command_buffer_tail()) {
        case KC_C:
            action |= VIM_MOD_CHANGE;
            break;
        case KC_D:
            action |= VIM_MOD_DELETE;
            break;
        case KC_Y:
            action |= VIM_MOD_YANK;
            break;
        default:
            send_type = record->event.pressed ? VIM_SEND_PRESS : VIM_SEND_RELEASE;
            break;
    }

    vim_clear_command();
    vim_perform_action(action, send_type);
}

void process_vim_visual(uint16_t keycode, keyrecord_t *record) {
    vim_dprintf_key(__func__, keycode, record);

    if (record->event.pressed) {
        if (vim_mods == 0) {
            switch (keycode) {
                case KC_ESC:
                    vim_enter_command_mode();
                    return;
                case KC_C:
                    vim_delete_os_selection();
                    vim_enter_insert_mode();
                    return;
                case KC_D:
                case KC_X:
                    vim_delete_os_selection();
                    return;
                case KC_G:
                    if (vim_command_buffer_tail() == KC_G) {
                        vim_clear_command();
                        vim_perform_action(VIM_ACTION_DOCUMENT_START | VIM_MOD_SELECT, VIM_SEND_TAP);
                        return;
                    } else {
                        vim_append_command(KC_G);
                        return;
                    }
                case KC_Y:
                    vim_yank_os_selection();
                    return;
            }
        } else if (vim_mods & MOD_MASK_SHIFT) {
            switch (keycode) {
                case KC_C:
                    vim_delete_line();
                    vim_enter_insert_mode();
                case KC_D:
                case KC_X:
                    vim_delete_line();
                    return;
                case KC_Y:
                    vim_yank_line();
                    return;
            }
        }
    }

    vim_send_type_t send_type = record->event.pressed ? VIM_SEND_PRESS : VIM_SEND_RELEASE;
    vim_action_t    action    = vim_get_action(keycode);
    vim_perform_action(action | VIM_MOD_SELECT, send_type);
}

bool process_record_vim(uint16_t keycode, keyrecord_t *record, uint16_t vim_keycode) {
    if (record->event.pressed) {
        static bool     tapped = false;
        static uint16_t timer  = 0;
        if (keycode == vim_keycode) {
            if (tapped && !timer_expired(record->event.time, timer)) {
                // double-tapped the vim key, toggle command mode
                vim_toggle_command_mode();
                return false;
            }
            VIM_DPRINT("Vim key pressed\n");
            tapped          = true;
            timer           = record->event.time + GET_TAPPING_TERM(keycode, record);
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

    if (vim_mode != VIM_INSERT_MODE) {
        if (IS_MODIFIERS_KEYCODE(keycode)) {
            if (record->event.pressed) {
                vim_mods |= MOD_BIT(keycode);
            } else {
                vim_mods &= ~MOD_BIT(keycode);
            }
        }
    }

    if (vim_mode == VIM_COMMAND_MODE || vim_key_pressed) {
        process_vim_command(keycode, record);
        return false;
    } else if (vim_mode == VIM_VISUAL_MODE) {
        process_vim_visual(keycode, record);
        return false;
    }

    return true;
}

bool vim_is_active_key(uint16_t keycode) {
    if (vim_mode == VIM_INSERT_MODE && !vim_key_pressed) {
        return false;
    }

    // keep in sync with vim_get_action
    if (vim_mods == 0) {
        switch (keycode) {
            case KC_B:
            case KC_E:
            case KC_H:
            case KC_J:
            case KC_K:
            case KC_L:
            case KC_P:
            case KC_U:
            case KC_W:
            case KC_X:
            case KC_0:
                return true;
            default:
                break;
        }
    } else if (vim_mods & MOD_MASK_SHIFT) {
        switch (keycode) {
            case KC_4: /*$*/
            case KC_6: /*^*/
            case KC_B:
            case KC_E:
            case KC_G:
            case KC_P:
            case KC_W:
            case KC_X:
                return true;
            default:
                break;
        }
    } else if (vim_mods & MOD_MASK_CTRL) {
        switch (keycode) {
            case KC_B:
            case KC_F:
                return true;
            default:
                break;
        }
    }

    if (vim_mode == VIM_COMMAND_MODE) {
        if (vim_mods == 0) {
            switch (keycode) {
                case KC_1 ... KC_9:
                case KC_A:
                case KC_C:
                case KC_D:
                case KC_G:
                case KC_I:
                case KC_V:
                case KC_Y:
                    return true;
                default:
                    return false;
            }
        } else if (vim_mods & MOD_MASK_SHIFT) {
            switch (keycode) {
                case KC_A:
                case KC_C:
                case KC_D:
                case KC_I:
                case KC_V:
                case KC_Y:
                    return true;
                default:
                    return false;
            }
        }
    } else if (vim_mode == VIM_VISUAL_MODE) {
        if (vim_mods == 0) {
            switch (keycode) {
                case KC_ESC:
                case KC_C:
                case KC_D:
                case KC_X:
                case KC_G:
                case KC_Y:
                    return true;
                default:
                    return false;
            }
        } else if (vim_mods & MOD_MASK_SHIFT) {
            switch (keycode) {
                case KC_C:
                case KC_D:
                case KC_X:
                case KC_Y:
                    return true;
                default:
                    return false;
            }
        }
    }

    return false;
}

vim_mode_t vim_get_mode(void) {
    return vim_mode;
}