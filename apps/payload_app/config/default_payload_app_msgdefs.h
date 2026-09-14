#ifndef DEFAULT_PAYLOAD_APP_MSGDEFS_H
#define DEFAULT_PAYLOAD_APP_MSGDEFS_H

#include "cfe_sb.h"
#include "payload_app_platform_cfg.h"

/*
** Command function codes are defined in payload_app_fcncodes.h
*/

typedef struct {
  CFE_MSG_CommandHeader_t command_header;
} PAYLOAD_APP_NoArgsCmd_t;

typedef struct {
  CFE_MSG_CommandHeader_t command_header;
  char                    file_name[PAYLOAD_APP_FILENAME_LEN];
} PAYLOAD_APP_ReadFileCmd_t;

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

  char   last_file[PAYLOAD_APP_FILENAME_LEN];
} PAYLOAD_APP_HkPacket_t;

#endif
