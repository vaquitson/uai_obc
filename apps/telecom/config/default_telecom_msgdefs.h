#ifndef DEFAULT_TELECOM_MSGDEFS_H
#define DEFAULT_TELECOM_MSGDEFS_H

#include "common_types.h"

typedef struct {
  uint8 command_counter;
} TELECOM_HkTlm_Payload_t;

#ifdef COMMUNICATION_LORA
typedef struct {
  char listening_freq[16];
  char sending_freq[16];
} TELECOM_EnableOutput_Payload_t;

#else
typedef struct {
  char dest_IP[16];
} TELECOM_EnableOutput_Payload_t;

#endif


#endif
