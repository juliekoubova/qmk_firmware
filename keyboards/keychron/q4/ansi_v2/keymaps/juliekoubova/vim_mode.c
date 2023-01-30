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

#define VIM_DPRINT(s)    dprint("[vim] " s)
#define VIM_DPRINTF(...) dprintf("[vim] " __VA_ARGS__)

typedef enum {
    VIM_INSERT_MODE,
    VIM_COMMAND_MODE,
    VIM_VISUAL_MODE,
} vim_mode_t;

typedef enum {
    VIM_MOTION_NONE,
    VIM_MOTION_LEFT,
    VIM_MOTION_DOWN,
    VIM_MOTION_UP,
    VIM_MOTION_RIGHT,
    VIM_MOTION_LINE_START,
    VIM_MOTION_LINE_END,
    VIM_MOTION_WORD_START,
    VIM_MOTION_WORD_END,
    VIM_MOTION_DOCUMENT_START,
    VIM_MOTION_DOCUMENT_END,
    VIM_MOTION_PAGE_UP,
    VIM_MOTION_PAGE_DOWN,
    VIM_MOTION_DELETE_LEFT,
    VIM_MOTION_DELETE_RIGHT,
} vim_motion_t;

typedef enum {
    VIM_MOTION_TYPE_TAP,
    VIM_MOTION_TYPE_PRESS,
    VIM_MOTION_TYPE_RELEASE,
} vim_motion_type_t;

#ifndef VIM_COMMAND_BUFFER_SIZE
#   define VIM_COMMAND_BUFFER_SIZE 10
#endif

static vim_mode_t vim_mode = VIM_INSERT_MODE;
static bool vim_key_pressed = false;

// track modifier state separately. we modify the actual mods so we can't rely on them
static uint8_t vim_mods = 0;

static uint8_t vim_command_buffer[VIM_COMMAND_BUFFER_SIZE] = {};
static uint8_t vim_command_buffer_size = 0;

void vim_dprintf_key(const char *function, uint16_t keycode, keyrecord_t *record) {
    VIM_DPRINTF(
        "%s vim_mods=%d keycode=%d pressed=%d\n", 
        function,
        vim_mods,
        keycode,
        record->event.pressed
    );
}

void vim_cancel_os_selection(void) {
    tap_code(KC_LEFT);
}

void vim_delete_os_selection(void) {
    tap_code16(C(KC_X));
}

void vim_yank_os_selection(void) {
    tap_code16(C(KC_C));
}

void vim_enter_insert_mode(void) {
    if (vim_mode == VIM_INSERT_MODE) {
        return;
    }
    VIM_DPRINT("Entering insert mode\n");
    vim_mode = VIM_INSERT_MODE;
    vim_mods = 0;
}

void vim_enter_command_mode(void) {
    if (vim_mode == VIM_COMMAND_MODE) {
        return;
    } else if (vim_mode == VIM_VISUAL_MODE) {
        vim_cancel_os_selection();
    }
    vim_mode = VIM_COMMAND_MODE;
    vim_mods = get_mods();
    VIM_DPRINTF("Entering command mode vim_mods=%d\n", vim_mods);
}

void vim_enter_visual_mode(void) {
    if (vim_mode == VIM_VISUAL_MODE) {
        return;
    }
    vim_mode = VIM_VISUAL_MODE;
    VIM_DPRINT("Entering visual mode\n");
}

void vim_toggle_command_mode(void) {
    if (vim_mode == VIM_INSERT_MODE) {
        vim_enter_command_mode();
    } else {
        vim_enter_insert_mode();
    }
}

void vim_perform_motion_in_mode(vim_motion_t motion, vim_motion_type_t type, vim_mode_t mode) {
    uint16_t keycode = KC_NO;
    uint8_t mods = 0;

    switch (motion) {
		case VIM_MOTION_LEFT: keycode = KC_LEFT; break;
		case VIM_MOTION_DOWN: keycode = KC_DOWN; break;
		case VIM_MOTION_UP: keycode = KC_UP; break;
		case VIM_MOTION_RIGHT: keycode = KC_RIGHT; break;
		case VIM_MOTION_LINE_START: keycode = KC_HOME; break;
		case VIM_MOTION_LINE_END: keycode = KC_END; break;
		case VIM_MOTION_WORD_START: keycode = KC_LEFT; mods = MOD_LCTL; break;
		case VIM_MOTION_WORD_END: keycode = KC_RIGHT; mods = MOD_LCTL; break;
		case VIM_MOTION_DOCUMENT_START: keycode = KC_HOME; mods = MOD_LCTL; break;
		case VIM_MOTION_DOCUMENT_END: keycode = KC_END; mods = MOD_LCTL; break;
		case VIM_MOTION_PAGE_UP: keycode = KC_PAGE_UP; break;
		case VIM_MOTION_PAGE_DOWN: keycode = KC_PAGE_DOWN; break;
		case VIM_MOTION_DELETE_LEFT: keycode = KC_BSPC; break;
		case VIM_MOTION_DELETE_RIGHT: keycode = KC_DEL; break;
        default: return;
    }

    if (mode == VIM_VISUAL_MODE) {
        mods |= MOD_LSFT;
    }

    if (mods && (type == VIM_MOTION_TYPE_PRESS || type == VIM_MOTION_TYPE_TAP)) {
        VIM_DPRINTF("register mods=%d\n", mods);
        register_mods(mods);
    }

    switch (type) {
        case VIM_MOTION_TYPE_TAP:
            VIM_DPRINTF("tap keycode=%d\n", keycode);
            tap_code(keycode);
            break;
        case VIM_MOTION_TYPE_PRESS:
            VIM_DPRINTF("register keycode=%d\n", keycode);
            register_code(keycode);
            break;
        case VIM_MOTION_TYPE_RELEASE:
            VIM_DPRINTF("unregister keycode=%d\n", keycode);
            unregister_code(keycode);
            break;
    }

    if (mods && (type == VIM_MOTION_TYPE_RELEASE || type == VIM_MOTION_TYPE_TAP)) {
        VIM_DPRINTF("unregister mods=%d\n", mods);
        unregister_mods(mods);
    }
}

void vim_perform_motion(vim_motion_t motion, vim_motion_type_t type) {
    vim_perform_motion_in_mode(motion, type, vim_mode);
}


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

vim_motion_t vim_get_motion(uint16_t keycode) {
    if (vim_mods == 0) {
        switch (keycode) {
            case KC_B: return VIM_MOTION_WORD_START;
            case KC_E:
            case KC_W: return VIM_MOTION_WORD_END;
            case KC_H: return VIM_MOTION_LEFT;
            case KC_J: return VIM_MOTION_DOWN;
            case KC_K: return VIM_MOTION_UP;
            case KC_L: return VIM_MOTION_RIGHT;
            case KC_X: return VIM_MOTION_DELETE_LEFT;
            case KC_0: return VIM_MOTION_LINE_START;
            default: return VIM_MOTION_NONE;
        }
    } else if (vim_mods & MOD_MASK_SHIFT) {
        switch (keycode) {
            case KC_4: /*$*/ return VIM_MOTION_LINE_END;
            case KC_6: /*^*/ return VIM_MOTION_LINE_START;
            case KC_B: return VIM_MOTION_WORD_START;
            case KC_E:
            case KC_W: return VIM_MOTION_WORD_END;
            case KC_X: return VIM_MOTION_DELETE_RIGHT;
            default: return VIM_MOTION_NONE;
        }
    } else if (vim_mods & MOD_MASK_CTRL) {
        switch (keycode) {
            case KC_B: return VIM_MOTION_PAGE_UP;
            case KC_F: return VIM_MOTION_PAGE_DOWN;
            default: return VIM_MOTION_NONE;
        }
    } else {
        return VIM_MOTION_NONE;
    }
}

void vim_delete_line(void) {
    vim_perform_motion_in_mode(
        VIM_MOTION_LINE_START,
        VIM_MOTION_TYPE_TAP,
        VIM_COMMAND_MODE
    );
    vim_perform_motion(VIM_MOTION_LINE_END, VIM_MOTION_TYPE_TAP);
    vim_delete_os_selection();
}

void vim_yank_line(void) {
    vim_perform_motion_in_mode(
        VIM_MOTION_LINE_START,
        VIM_MOTION_TYPE_TAP,
        VIM_COMMAND_MODE
    );
    vim_perform_motion(VIM_MOTION_LINE_END, VIM_MOTION_TYPE_TAP);
    vim_yank_os_selection();
    vim_perform_motion_in_mode(
        VIM_MOTION_LINE_START,
        VIM_MOTION_TYPE_TAP,
        VIM_COMMAND_MODE
    );
}

void process_vim_command(uint16_t keycode, keyrecord_t *record) {
    vim_dprintf_key(__func__, keycode, record);
    if (record->event.pressed) {
        if (vim_mods == 0) {
            switch (keycode) {
                case KC_1 ... KC_9:
                case KC_C:
                case KC_D:
                    vim_append_command(keycode);
                    return;
                case KC_A:
                    vim_perform_motion(VIM_MOTION_RIGHT, VIM_MOTION_TYPE_TAP);
                    vim_enter_insert_mode();
                    return;
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
                    vim_perform_motion(VIM_MOTION_LINE_END, VIM_MOTION_TYPE_TAP);
                    vim_enter_insert_mode();
                    return;
                case KC_I:
                    vim_perform_motion(VIM_MOTION_LINE_START, VIM_MOTION_TYPE_TAP);
                    vim_enter_insert_mode();
                    return;
                case KC_V:
                    vim_perform_motion(VIM_MOTION_LINE_START, VIM_MOTION_TYPE_TAP);
                    vim_enter_visual_mode();
                    vim_perform_motion(VIM_MOTION_LINE_END, VIM_MOTION_TYPE_TAP);
                    return;
                case KC_Y:
                    vim_yank_line();
                    return;
            }
        }
    }

    vim_motion_t motion = vim_get_motion(keycode);
    if (motion != VIM_MOTION_NONE) {
        vim_perform_motion(
            motion,
            record->event.pressed ? VIM_MOTION_TYPE_PRESS : VIM_MOTION_TYPE_RELEASE
        );
    }
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

    vim_motion_t motion = vim_get_motion(keycode);
    if (motion != VIM_MOTION_NONE) {
        vim_perform_motion(
            motion,
            record->event.pressed ? VIM_MOTION_TYPE_PRESS : VIM_MOTION_TYPE_RELEASE
        );
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

