###########################################################
#
# OBC_HK mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

set(OBC_HK_MISSION_CONFIG_FILE_LIST
  obc_hk_msg.h
  obc_hk_msgid_values.h
  obc_hk_msgids.h
  obc_hk_topicid_values.h
  obc_hk_msgdefs.h
)

generate_configfile_set(${OBC_HK_MISSION_CONFIG_FILE_LIST})

