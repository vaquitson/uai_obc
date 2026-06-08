###########################################################
#
# SAMPLE_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SAMPLE_APP configuration
set(OBC_HK_PLATFORM_CONFIG_FILE_LIST
  obc_hk_msgids.h
  obc_hk_msgid_values.h
)

generate_configfile_set(${OBC_HK_PLATFORM_CONFIG_FILE_LIST})

