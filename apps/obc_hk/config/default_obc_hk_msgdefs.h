#ifndef DEFAUTL_OBC_HK_MSGFFS_H
#define DEFAUTL_OBC_HK_MSGFFS_H

#include "cfe_sb.h"

typedef struct {
  uint8          cmd_counter;
  uint8          err_counter;
} OBC_HK_KkTlm_Payload_t;


typedef struct {
    float    cpu_temp;
    float    ram_usage_percent;
    long int ram_usage;
    float    cpu_usage;
} OBC_HK_ObcInfoTlm_Payload_t;

typedef struct {
    float cpu_temp;
} OBC_HK_CpuTempTlm_Payload_t;

typedef struct {
    float    ram_usage_percent;
    long int ram_usage;
} OBC_HK_RamUsageTlm_Payload_t;

typedef struct {
    float cpu_usage;
} OBC_HK_CpuUsageTlm_Payload_t;

#endif
