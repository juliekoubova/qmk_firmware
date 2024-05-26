#pragma once
#include <stdint.h>

void vim_append_command(uint8_t keycode);
void vim_clear_command(void);
uint8_t vim_command_buffer_tail(void);
