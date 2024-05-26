ifeq ($(strip $(VIM_MODE_ENABLE)), yes)
  SRC += users/juliekoubova/vim.c
  SRC += users/juliekoubova/vim_command_buffer.c
  SRC += users/juliekoubova/vim_mode.c
  SRC += users/juliekoubova/vim_send.c
  SRC += users/juliekoubova/vim_statemachine.c
  SRC += users/juliekoubova/vim_windows.c
endif
