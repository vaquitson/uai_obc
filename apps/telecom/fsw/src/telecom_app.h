
#ifndef TELECOM_H
#define TELECOM_H

#include "cfe_es.h"
#include "cfe_sb.h"

void  SAMPLE_APP_Main(void);
int   TELECOM_APP_Init(void);

#define TELECOM_TLM_PIP_NAME "TELECOM_TLM_PIPE"
#define TELECOM_CMD_PIP_NAME "TELECOM_CMD_PIPE"

typedef uint32 TELECOM_Err_t;

typedef struct {
  char tlm_pip_name[17];
  CFE_SB_PipeId_t tlm_pipe;

  char cmd_pip_name[17];
  CFE_SB_PipeId_t cmd_pipe;


  uint32 run_status;

} TELECOM_GlobalApp_t;

#endif
