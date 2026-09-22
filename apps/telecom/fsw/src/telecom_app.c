#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cfe_evs.h"

#include "obc_hk_msg.h"
#include "obc_hk_msgdefs.h"

#include "cmd_hand_msgids.h"

#include "telecom_interface_cfg.h"
#include "telecom_internal_cfg.h"
#include "telecom_msgids.h"
#include "telecom_eventids.h"
#include "telecom_dispatch.h"
#include "telecom_errors.h"
#include "telecom_tlm.h"
#include "telecom_app.h"
#include "telecom_encode.h"

TELECOM_GlobalApp_t TELECOM_data;

CFE_Status_t TELECOM_APP_Init(void){ 
  CFE_Status_t status;

  memset(&TELECOM_data, 0, sizeof(TELECOM_data));
  TELECOM_data.suppress_sendto = false;
  TELECOM_data.downlink_on = false;

  TELECOM_data.run_status = CFE_ES_RunStatus_APP_RUN; 

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS){
    CFE_ES_WriteToSysLog("TELECOM: Error registering for Event Services, RC = 0x%08X\n", (unsigned int)status);
  }
  
  strncpy(TELECOM_data.tlm_pipe_name, TELECOM_TLM_PIPE_NAME, TELECOM_TLM_PIPE_NAME_MAX);
  status = CFE_SB_CreatePipe(&TELECOM_data.tlm_pipe, 10, TELECOM_data.tlm_pipe_name);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_PIPE_CREATION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to properly crate the tlm pipe,  RC = 0x%08lX", (unsigned long)status);

  status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(OBC_HK_TLM_MID), TELECOM_data.tlm_pipe);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_SUBSCRIPTION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to subscribe to OBC HK,  RC = 0x%08lX", (unsigned long)status);

  strncpy(TELECOM_data.cmd_pipe_name, TELECOM_CMD_PIPE_NAME, TELECOM_CMD_PIPE_NAME_MAX);
  status = CFE_SB_CreatePipe(&TELECOM_data.cmd_pipe, 10, TELECOM_data.cmd_pipe_name);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_PIPE_CREATION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to properly crate the cmd pipe,  RC = 0x%08lX", (unsigned long)status);
 

  status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(TELECOM_CMD_MID), TELECOM_data.tlm_pipe);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_SUBSCRIPTION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to subscribe to TELECOM CMD MID,  RC = 0x%08lX", (unsigned long)status);

  TELECOM_APP_open_telemetry();

  if (status == CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_INIT_SUCCESFULL_EID, CFE_EVS_EventType_INFORMATION,
                      "TELECOM: Initialized succesfuly, RC = 0x%08lX", (unsigned long)status);

  return status;
}

void TELECOM_process_cmd(void){
  CFE_Status_t status;
  CFE_SB_Buffer_t *sb_buf_p;

  while (1){
    status = CFE_SB_ReceiveBuffer(&sb_buf_p, TELECOM_data.cmd_pipe, CFE_SB_POLL);
    if (status != CFE_SUCCESS){
      break;
    }
    TELECOM_task_pipe(sb_buf_p);
  }
}



void TELECOM_AppMain(void){
  if (TELECOM_APP_Init() != CFE_SUCCESS){
    TELECOM_data.run_status = CFE_ES_RunStatus_APP_ERROR;     
  }

  while (CFE_ES_RunLoop(&TELECOM_data.run_status) == true){
    OS_TaskDelay(TELECOM_PLATFORM_TASK_MSEC);
    TELECOM_process_cmd();
    TELECOM_APP_forward_telemetry(); 
  }

  CFE_ES_ExitApp(TELECOM_data.run_status);
}

