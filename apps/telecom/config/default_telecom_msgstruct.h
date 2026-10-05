#ifndef DEFAULT_TELECOM_MSGSTRUCT_H
#define DEFAULT_TELECOM_MSGSTRUCT_H

#include "cfe_msg.h"
#include "telecom_msgdefs.h"

typedef struct {
  CFE_MSG_CommandHeader_t CommandHeader;
} TELECOM_SendHkCmd_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t TelemetryHeader;
  TELECOM_HkTlm_Payload_t   payload;
} TELECOM_HkTlm_t;

typedef struct { 
  CFE_MSG_CommandHeader_t CommandHeader;
} TELECOM_NoopCmd_t;


// #### Open Tlm
typedef struct {
  CFE_MSG_CommandHeader_t CommandHeader;
  TELECOM_OpenTlmCmd_Payload_t payload;
} TELECOM_OpenTlmCmd_t;

typedef struct {
  CFE_MSG_TelemetryHeader_t TelemetryHeader;
  TELECOM_OpenTlmTlm_Payload_t payload;
} TELECOM_OpenTlmTlm_t;


#endif


