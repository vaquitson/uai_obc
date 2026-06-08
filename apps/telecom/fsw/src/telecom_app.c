#include "telecom_app.h"
#include "cfe_evs.h"
#include "telecom_eventids.h"
#include "telecom_errors.h"
#include <string.h>

TELECOM_GlobalApp_t TELECOM_Global;

TELECOM_Err_t TELECOM_PipesInit(void){
  CFE_Status_t status; 

  status = CFE_SB_CreatePipe(&TELECOM_Global.tlm_pipe, 10, TELECOM_Global.tlm_pip_name);
  if (status != CFE_SUCCESS){
    return TELECOM_ERR_TLM_PIPE;
  }

  status = CFE_SB_CreatePipe(&TELECOM_Global.cmd_pipe, 10, TELECOM_Global.cmd_pip_name);
  if (status != CFE_SUCCESS){
    return TELECOM_ERR_CMD_PIPE;
  }

  return TELECOM_SUCCESS;
}


void TELECOM_Process_commands(void){

}

void TELECOM_Forward_tlm(void){

}




CFE_Status_t TELECOM_APP_Init(void){ 
  CFE_Status_t status;

  // start the event service
  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS){
    CFE_ES_WriteToSysLog("TELECOM: Error registering for Event Services, RC = 0x%08X\n", (unsigned int)status);
  }
  
  strncpy(TELECOM_Global.tlm_pip_name, TELECOM_TLM_PIP_NAME, 17);  
  strncpy(TELECOM_Global.cmd_pip_name, TELECOM_CMD_PIP_NAME, 17);

  TELECOM_Global.run_status = CFE_ES_RunStatus_APP_RUN; 
  
  CFE_EVS_SendEvent(TELECOM_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "Telecom Initialized");

  return CFE_SUCCESS;
}


void TELECOM_AppMain(void){
  if (TELECOM_APP_Init() != CFE_SUCCESS){
    TELECOM_Global.run_status = CFE_ES_RunStatus_APP_ERROR;     
  }

  while (CFE_ES_RunLoop(&TELECOM_Global.run_status) == true){

  }
}

