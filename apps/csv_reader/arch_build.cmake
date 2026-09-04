###########################################################
#
# CSV_READER platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the CSV_READER configuration
set(CSV_READER_PLATFORM_CONFIG_FILE_LIST
  csv_reader_msgids.h
  csv_reader_msgdefs.h
  csv_reader_platform_cfg.h
)

generate_configfile_set(${CSV_READER_PLATFORM_CONFIG_FILE_LIST})
