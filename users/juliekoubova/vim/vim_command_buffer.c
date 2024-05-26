#include "vim_command_buffer.h"
#include "debug.h"
#include "quantum/quantum.h"
#include <stdint.h>

#ifndef VIM_COMMAND_BUFFER_SIZE
#    define VIM_COMMAND_BUFFER_SIZE 10
#endif

static uint8_t vim_command_buffer[VIM_COMMAND_BUFFER_SIZE] = {};
static uint8_t vim_command_size                     = 0;

void vim_append_command(uint8_t keycode) {
    if (vim_command_size == VIM_COMMAND_BUFFER_SIZE) {
        VIM_DPRINT("[vim] Ran out of command buffer space.\n");
        vim_command_size = 0;
        return;
    }
    vim_command_buffer[vim_command_size] = keycode;
    vim_command_size++;

#ifdef VIM_DEBUG
    dprint("[vim] vim_command_buffer: { ");
    for (uint8_t i = 0; i < vim_command_size; i++) {
        dprintf("%d ", vim_command_buffer[i]);
    }
    dprint("}\n");
#endif
}

void vim_clear_command(void) {
    VIM_DPRINTF("vim_command_buffer={}\n");
    vim_command_size = 0;
}

uint8_t vim_command_buffer_tail(void) {
    if (vim_command_size == 0) {
        return KC_NO;
    } else {
        return vim_command_buffer[vim_command_size - 1];
    }
}
