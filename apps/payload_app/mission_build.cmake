###########################################################
#
# PAYLOAD_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

set(PAYLOAD_APP_MISSION_CONFIG_FILE_LIST
  payload_app_msg.h
  payload_app_msgid_values.h
  payload_app_msgids.h
  payload_app_topicid_values.h
  payload_app_msgdefs.h
  payload_app_platform_cfg.h
)

generate_configfile_set(${PAYLOAD_APP_MISSION_CONFIG_FILE_LIST})
