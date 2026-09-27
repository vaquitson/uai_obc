#ifndef PAYLOAD_APP_DATA_H
#define PAYLOAD_APP_DATA_H

#include <stddef.h>
#include "payload_app_msgdefs.h"

/* Parses one Payload response envelope ({request_id, status, status_code,
** data}), publishes it as PAYLOAD_APP_DATA_TLM_MID on the Software Bus, and
** prints the same populated message struct to stdout right after, so both
** outputs are guaranteed to show identical field values. */
void PAYLOAD_APP_ParseAndPublish(const char *json, size_t len);

/* Prints every field of a PAYLOAD_APP_DataMsg_t (except the header), in the
** same order they were placed into the struct that gets transmitted to the SB. */
void PAYLOAD_APP_PrintDataMsg(const PAYLOAD_APP_DataMsg_t *msg);

#endif
