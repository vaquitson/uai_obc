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
// #### END ####

// #### NOOP ####
typedef struct {
  CFE_MSG_CommandHeader_t CommandHeader;
} OBC_HK_NoopCmd_t;
// #### END ####

// #### OBC INFO ####
typedef struct {
  CFE_MSG_CommandHeader_t CommandHeader;
} OBC_HK_SendObcInfoCmd_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t TelemetryHeader;
  OBC_HK_SendObcInfo_Payload_t payload;
} OBC_HK_SendObcInfoTlm_t;
// #### END ####


#endif
