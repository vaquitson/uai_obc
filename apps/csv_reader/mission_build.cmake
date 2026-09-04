###########################################################
#
# CSV_READER mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

set(CSV_READER_MISSION_CONFIG_FILE_LIST
  csv_reader_msg.h
  csv_reader_msgid_values.h
  csv_reader_msgids.h
  csv_reader_topicid_values.h
  csv_reader_msgdefs.h
  csv_reader_platform_cfg.h
)

generate_configfile_set(${CSV_READER_MISSION_CONFIG_FILE_LIST})
