#include "debug.h"
#include "statemachine.h"
#include "vim_mode.h"

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
    VIM_ACTION(KC_C, VIM_ACTION_SELECTION | VIM_MOD_CHANGE),
    VIM_ACTION(KC_D, VIM_ACTION_SELECTION | VIM_MOD_DELETE | VIM_MOD_COMMAND_AFTER),
    VIM_ACTION_REPEATING(KC_E, VIM_ACTION_WORD_END | VIM_MOD_SELECT),
    VIM_BUFFER_THEN_ACTION(KC_G, VIM_ACTION_DOCUMENT_START | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_H, VIM_ACTION_LEFT | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_J, VIM_ACTION_DOWN | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_K, VIM_ACTION_UP | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_L, VIM_ACTION_RIGHT | VIM_MOD_SELECT),
    VIM_ACTION_REPEATING(KC_P, VIM_ACTION_PASTE),
    VIM_ACTION(KC_S, VIM_ACTION_SELECTION | VIM_MOD_CHANGE),
    VIM_ACTION(KC_V, VIM_ACTION_COMMAND_MODE),
    VIM_ACTION_REPEATING(KC_W, VIM_ACTION_WORD_END | VIM_MOD_SELECT),
    VIM_ACTION(KC_X, VIM_ACTION_SELECTION | VIM_MOD_DELETE),
    VIM_ACTION(KC_Y, VIM_ACTION_SELECTION | VIM_MOD_YANK | VIM_MOD_COMMAND_AFTER),
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

const vim_statemachine_t *vim_lookup_statemachine(uint16_t keycode) {
    if (keycode < VIM_STATEMACHINE_FIRST || keycode > VIM_STATEMACHINE_LAST) {
        return NULL;
    }

    uint8_t vim_mods = vim_get_mods();
    uint8_t index = VIM_STATEMACHINE_INDEX(keycode);
    bool    ctrl  = (vim_mods & MOD_BIT(KC_LCTL)) || (vim_mods & MOD_BIT(KC_RCTL));
    bool    shift = (vim_mods & MOD_BIT(KC_LSFT)) || (vim_mods & MOD_BIT(KC_RSFT));

    switch (vim_get_mode()) {
        case VIM_MODE_COMMAND:
            if (ctrl) {
                return &vim_statemachine_command_ctrl[index];
            } else if (shift) {
                return &vim_statemachine_command_shift[index];
            } else if (vim_mods == 0) {
                return &vim_statemachine_command[index];
            }
            break;
        case VIM_MODE_VISUAL:
            if (vim_mods == 0) {
                return &vim_statemachine_visual[index];
            } else if (shift) {
                return &vim_statemachine_visual_shift[index];
            }
            break;

        default:
            break;
    }

    return NULL;
}

bool vim_is_active_key(uint16_t keycode) {
    if (vim_get_mode() == VIM_MODE_INSERT) {
        return false;
    }

    const vim_statemachine_t *state = vim_lookup_statemachine(keycode);
    return state != NULL && state->action != VIM_ACTION_NONE;
}

void vim_dprintf_state(const vim_statemachine_t *state) {
    VIM_DPRINTF("state=%s\n", state ? state->debug : "NULL");
}

