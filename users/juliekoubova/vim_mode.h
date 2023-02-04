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

typedef enum {
    VIM_INSERT_MODE,
    VIM_COMMAND_MODE,
    VIM_VISUAL_MODE,
} vim_mode_t;

vim_mode_t vim_get_mode(void);
void vim_mode_changed(vim_mode_t mode);

// Returns true for keys that are mapped in the current VIM mode.
// Useful for indicating the current mode using RGB matrix lights.
bool vim_is_active_key(uint16_t keycode);