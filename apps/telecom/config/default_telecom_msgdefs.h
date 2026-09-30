#ifndef DEFAULT_TELECOM_MSGDEFS_H
#define DEFAULT_TELECOM_MSGDEFS_H

#include "common_types.h"

typedef struct {
  uint8 command_counter;
  uint8 err_counter;
} TELECOM_HkTlm_Payload_t;

#ifdef COMMUNICATION_LORA
typedef struct {
  char downlink_freq[16];
  char uplink_freq[16];
} TELECOM_OpenTlmCmd_Payload_t;

#else
typedef struct {
  char dest_IP[16];
  char dest_port[16];
} TELECOM_OpenTlmCmd_Payload_t;

#endif

typedef struct {
  uint32 status_code;
} TELECOM_OpenTlmTlm_Payload_t;

#endif
