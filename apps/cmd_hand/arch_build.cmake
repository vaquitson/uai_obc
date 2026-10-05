
set(CMD_HAND_PLATFORM_CONFIG_FILE_LIST
  cmd_hand_msgids.h
  cmd_hand_msg.h
  cmd_hand_msgid.h
  cmd_hand_msgstruct.h
  cmd_hand_msgdefs.h
)

generate_configfile_set(${CMD_HAND_PLATFORM_CONFIG_FILE_LIST})
