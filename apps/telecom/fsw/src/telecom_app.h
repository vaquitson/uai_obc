#ifndef TELECOM_H
#define TELECOM_H

#include "cfe_es.h"
#include "cfe_sb.h"

void  SAMPLE_APP_Main(void);
int   TELECOM_APP_Init(void);

#define TELECOM_TLM_PIPE_NAME "TELECOM_TLM_PIPE"
#define TELECOM_TLM_PIPE_NAME_MAX 17

typedef uint32 TELECOM_Err_t;

typedef struct {
  char tlm_pip_name[TELECOM_TLM_PIPE_NAME_MAX];
  CFE_SB_PipeId_t tlm_pipe;

  uint32 run_status;

} TELECOM_GlobalApp_t;

#endif
