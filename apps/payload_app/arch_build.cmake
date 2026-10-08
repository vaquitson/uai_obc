###########################################################
#
# PAYLOAD_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the PAYLOAD_APP configuration
set(PAYLOAD_APP_PLATFORM_CONFIG_FILE_LIST
  payload_app_msgids.h
  payload_app_msgdefs.h
  payload_app_platform_cfg.h
)

generate_configfile_set(${PAYLOAD_APP_PLATFORM_CONFIG_FILE_LIST})
