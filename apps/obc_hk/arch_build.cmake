###########################################################
#
# OBC_HK platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the OBC_HK configuration
set(OBC_HK_PLATFORM_CONFIG_FILE_LIST
  obc_hk_msgids.h
  obc_hk_msgdefs.h
)

generate_configfile_set(${OBC_HK_PLATFORM_CONFIG_FILE_LIST})

