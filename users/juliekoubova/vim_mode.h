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

#pragma once
#include "quantum.h"

bool process_record_vim(uint16_t keycode, keyrecord_t *record, uint16_t vim_keycode);

enum {
    VIM_MODE_INSERT,
    VIM_MODE_COMMAND,
    VIM_MODE_VISUAL,
};

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
    VIM_ACTION_LINE,
    VIM_ACTION_PAGE_UP,
    VIM_ACTION_PAGE_DOWN,
    VIM_ACTION_WORD_START,
    VIM_ACTION_WORD_END,

    VIM_ACTION_PASTE,
    VIM_ACTION_UNDO,
    VIM_ACTION_COMMAND_MODE,
    VIM_ACTION_VISUAL_MODE,

    VIM_MOD_MOVE         = 0,
    VIM_MOD_CHANGE       = 0x0100,
    VIM_MOD_DELETE       = 0x0200,
    VIM_MOD_SELECT       = 0x0400,
    VIM_MOD_YANK         = 0x0800,
    VIM_MOD_INSERT_AFTER = 0x1000,
    VIM_MOD_VISUAL_AFTER = 0x2000,

    VIM_MASK_ACTION = 0x00ff,
    VIM_MASK_MOD    = 0xff00,
};

typedef uint16_t vim_action_t;

typedef uint8_t vim_mode_t;

vim_mode_t vim_get_mode(void);
void       vim_mode_changed(vim_mode_t mode);

// Returns true for keys that are mapped in the current VIM mode.
// Useful for indicating the current mode using RGB matrix lights.
bool vim_is_active_key(uint16_t keycode);