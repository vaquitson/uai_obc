#ifndef DEFAULT_OBC_HK_MSGSTRUCTS_H
#define DEFAULT_OBC_HK_MSGSTRUCTS_H

#include "cfe_sb.h"
#include "obc_hk_msgdefs.h"
#include "cfe_sb.h"

// #### House Keeping ####
typedef struct {
  CFE_MSG_CommandHeader_t CommandHeader;
} OBC_HK_HkCmd_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t TelemetryHeader;
  OBC_HK_KkTlm_Payload_t payload;
} OBC_HK_HkTlm_t;


// #### NOOP ####
typedef struct {
  CFE_MSG_CommandHeader_t CommandHeader;
} OBC_HK_NoopCmd_t;


// #### Send OBC INFO ####
typedef struct {
    CFE_MSG_TelemetryHeader_t       TelemetryHeader;
    OBC_HK_ObcInfoTlm_Payload_t payload;
} OBC_HK_ObcInfoTlm_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} OBC_HK_SendObcInfoCmd_t;

// #### SEND CPU TEMP
typedef struct {
    CFE_MSG_TelemetryHeader_t        TelemetryHeader;
    OBC_HK_CpuTempTlm_Payload_t  payload;
} OBC_HK_CpuTempTlm_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} OBC_HK_SendCpuTempCmd_t;

// Send Ram Usage
typedef struct {
    CFE_MSG_TelemetryHeader_t        TelemetryHeader;
    OBC_HK_RamUsageTlm_Payload_t payload;
} OBC_HK_RamUsageTlm_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} OBC_HK_SendRamUsageCmd_t;

// Send Cpu Usage
typedef struct {
    CFE_MSG_TelemetryHeader_t        TelemetryHeader;
    OBC_HK_CpuUsageTlm_Payload_t payload;
} OBC_HK_CpuUsageTlm_t;

typedef struct {
    CFE_MSG_CommandHeader_t CommandHeader;
} OBC_HK_SendCpuUsageCmd_t;

#endif
