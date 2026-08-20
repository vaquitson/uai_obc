project(CFE_CMD_HAND C)

set(APP_SRC_FILES
  fsw/src/cmd_hand.c)

add_cfe_app(cmd_hand ${APP_SRC_FILES})

target_include_directories(cmd_hand PUBLIC fsw/inc)
