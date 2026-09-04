#ifndef DEFAULT_CSV_READER_MSGDEFS_H
#define DEFAULT_CSV_READER_MSGDEFS_H

#include "cfe_sb.h"
#include "csv_reader_platform_cfg.h"

/*
** Command function codes are defined in csv_reader_fcncodes.h
*/

typedef struct {
  CFE_MSG_CommandHeader_t command_header;
} CSV_READER_NoArgsCmd_t;

typedef struct {
  CFE_MSG_CommandHeader_t command_header;
  char                    file_name[CSV_READER_FILENAME_LEN];
} CSV_READER_ReadFileCmd_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t telemetry_header;

  uint8  cmd_counter;
  uint8  err_counter;
  uint8  last_read_valid;
  uint8  spare;

  uint32 rows_read;
  uint32 cols_read;

  float  value_sum;
  float  value_min;
  float  value_max;

  char   last_file[CSV_READER_FILENAME_LEN];
} CSV_READER_HkPacket_t;

#endif
