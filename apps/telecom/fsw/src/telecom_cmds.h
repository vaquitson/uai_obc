#ifndef TELECOM_CMD_H
#define TELECOM_CMD_H

#include "telecom_msg.h"
#include "common_types.h"
#include "cfe_error.h"

CFE_Status_t TELECOM_noop_cmd(const TELECOM_NoopCmd_t *data_p);
  
#endif
