#ifndef DEFAULT_CMD_HAND_MSGSTRUCT_H
#define DEFAULT_CMD_HAND_MSGSTRUCT_H

#include "cfe_sb.h"
#include "cmd_hand_msgdefs.h"

// #### Send House Kepping
typedef struct {
  CFE_MSG_CommandHeader_t CommandHeader;
} CMD_HAND_SendHkCmd_t;


typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    CMD_HAND_HkTlm_Payload_t  payload;
} CMD_HAND_HkTlm_t;

#endif
