ifeq ($(strip $(VIM_MODE_ENABLE)), yes)
  SRC += vim_mode.c
endif