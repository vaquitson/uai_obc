###########################################################
#
# TELECOM platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the TELECOM app configuration
set(TELECOM_PLATFORM_CONFIG_FILE_LIST
)

generate_configfile_set(${TELECOM_PLATFORM_CONFIG_FILE_LIST})

