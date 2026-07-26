#ifndef DEFAUTL_OBC_HK_MSGFFS_H
#define DEFAUTL_OBC_HK_MSGFFS_H

#include "cfe_sb.h"

typedef struct {
  CFE_MSG_TelemetryHeader_t telemetry_header;
  float          cpu_temp;
  float          ram_usage_percent;
  long int       ram_usage;

  uint8          cmd_counter;
  uint8          err_counter;
} OBC_HK_HkPacket_t;


#endif
