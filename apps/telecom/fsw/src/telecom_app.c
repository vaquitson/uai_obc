#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cfe_evs.h"
#include "obc_hk_msg.h"
#include "telecom_eventids.h"
#include "telecom_errors.h"
#include "telecom_app.h"

TELECOM_GlobalApp_t TELECOM_Global;

CFE_Status_t TELECOM_APP_Init(void){ 
  CFE_Status_t status;

  TELECOM_Global.run_status = CFE_ES_RunStatus_APP_RUN; 

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS){
    CFE_ES_WriteToSysLog("TELECOM: Error registering for Event Services, RC = 0x%08X\n", (unsigned int)status);
  }
  
  strncpy(TELECOM_Global.tlm_pip_name, TELECOM_TLM_PIPE_NAME, TELECOM_TLM_PIPE_NAME_MAX);
  status = CFE_SB_CreatePipe(&TELECOM_Global.tlm_pipe, 10, TELECOM_Global.tlm_pip_name);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_PIPE_CREATION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to properly crate the tlm pipe,  RC = 0x%08lX", (unsigned long)status);
  
  if (status == CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_INIT_SUCCESFULL_EID, CFE_EVS_EventType_INFORMATION,
                      "TELECOM: Initialized succesfuly, RC = 0x%08lX", (unsigned long)status);

  return status;
}


void TELECOM_AppMain(void){
  if (TELECOM_APP_Init() != CFE_SUCCESS){
    TELECOM_Global.run_status = CFE_ES_RunStatus_APP_ERROR;     
  }

  while (CFE_ES_RunLoop(&TELECOM_Global.run_status) == true){
    OS_TaskDelay(1000);
  }
}

