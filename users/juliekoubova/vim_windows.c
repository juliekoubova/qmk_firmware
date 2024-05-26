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

#include "vim_command_buffer.h"
#include "vim_windows.h"
#include "quantum/keycode.h"
#include "vim_debug.h"
#include "vim_statemachine.h"

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
            vim_send(0, KC_HOME, VIM_SEND_TAP);
            type   = VIM_SEND_TAP;
            action = (action & VIM_MASK_MOD) | VIM_ACTION_LINE_END;
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
                break;
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

    VIM_DPRINTF("action=%x\n",  action);
    if (action & (VIM_MOD_DELETE | VIM_MOD_CHANGE)) {
        if (keycode != KC_NO) {
            // keycode is KC_NO in visual mode, where the object is the visual selection
            // send shifted action as a tap
            vim_send(mods | MOD_LSFT, keycode, VIM_SEND_TAP);
        }
        vim_send(MOD_LCTL, KC_X, VIM_SEND_TAP);
    } else if (action & VIM_MOD_YANK) {
        vim_send(mods | MOD_LSFT, keycode, VIM_SEND_TAP);
        vim_send(MOD_LCTL, KC_C, VIM_SEND_TAP);
    } else if (action & VIM_MOD_SELECT) {
        vim_send(mods | MOD_LSFT, keycode, type);
    } else {
        vim_send(mods, keycode, type);
    }

    if (action & (VIM_MOD_CHANGE | VIM_MOD_INSERT_AFTER)) {
        vim_enter_insert_mode();
    } else if (action & VIM_MOD_VISUAL_AFTER) {
        vim_enter_visual_mode();
    } else if (action & VIM_MOD_COMMAND_AFTER) {
        vim_enter_command_mode();
    }
}

