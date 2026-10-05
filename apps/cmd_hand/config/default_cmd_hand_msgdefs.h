#ifndef DEFAULT_CMD_HAND_MSGDEFS_H
#define DEFAULT_CMD_HAND_MSGDEFS_H

#include "common_types.h"

typedef struct {
    uint8 err_counter;
    uint8 cmd_counter;
    uint8 cmd_ingest;
} CMD_HAND_HkTlm_Payload_t;

#endif
