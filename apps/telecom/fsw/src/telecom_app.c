#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cfe_evs.h"


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
#include "obc_hk_msg.h"

// subscription array
uint32 TLM_SUBSCRIPTION_ARR[] = {
  OBC_HK_HK_MID,
  OBC_HK_OBC_INFO_MID,
  OBC_HK_CPU_TEMP_MID,
  OBC_HK_RAM_USAGE_MID,
  OBC_HK_CPU_USAGE_MID,
  TELECOM_OPEN_TLM_MID,
  TELECOM_HK_TLM_MID
};

uint32 CMD_SUBSCRIPTION_ARR[] = {
  TELECOM_CMD_MID,
  TELECOM_SEND_HK_MID
};

TELECOM_GlobalApp_t TELECOM_data;

CFE_Status_t TELECOM_init_pipes(void){
  CFE_Status_t status;

  strncpy(TELECOM_data.tlm_pipe_name, TELECOM_TLM_PIPE_NAME, TELECOM_TLM_PIPE_NAME_MAX);
  status = CFE_SB_CreatePipe(&TELECOM_data.tlm_pipe, 10, TELECOM_data.tlm_pipe_name);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_PIPE_CREATION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to properly crate the tlm pipe,  RC = 0x%08lX", (unsigned long)status);


  for (int i = 0; i < sizeof(TLM_SUBSCRIPTION_ARR)/sizeof(uint32); i++){
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(TLM_SUBSCRIPTION_ARR[i]), TELECOM_data.tlm_pipe);
    if (status != CFE_SUCCESS){
      CFE_EVS_SendEvent(TELECOM_SUBSCRIPTION_ERR_EID, CFE_EVS_EventType_ERROR,
                        "TELECOM: Faild to subscribe to %d,  RC = 0x%08lX",TLM_SUBSCRIPTION_ARR[i], (unsigned long)status);
    }
  }

  strncpy(TELECOM_data.cmd_pipe_name, TELECOM_CMD_PIPE_NAME, TELECOM_CMD_PIPE_NAME_MAX);
  status = CFE_SB_CreatePipe(&TELECOM_data.cmd_pipe, 10, TELECOM_data.cmd_pipe_name);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(TELECOM_PIPE_CREATION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "TELECOM: Faild to properly crate the cmd pipe,  RC = 0x%08lX", (unsigned long)status);

 for (int i = 0; i < sizeof(CMD_SUBSCRIPTION_ARR)/sizeof(uint32); i++){
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CMD_SUBSCRIPTION_ARR[i]), TELECOM_data.cmd_pipe);
    if (status != CFE_SUCCESS){
      CFE_EVS_SendEvent(TELECOM_SUBSCRIPTION_ERR_EID, CFE_EVS_EventType_ERROR,
                        "TELECOM: Faild to subscribe to %d,  RC = 0x%08lX", CMD_SUBSCRIPTION_ARR[i], (unsigned long)status);
    }
  }

  return status;
}


CFE_Status_t TELECOM_APP_init(void){ 
  CFE_Status_t status;

  memset(&TELECOM_data, 0, sizeof(TELECOM_data));

  TELECOM_data.run_status = CFE_ES_RunStatus_APP_RUN; 

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS){
    CFE_ES_WriteToSysLog("TELECOM: Error registering for Event Services, RC = 0x%08X\n", (unsigned int)status);
  }

  status = TELECOM_init_pipes();
  if (status != CFE_SUCCESS){
    CFE_EVS_SendEvent(TELECOM_PIPE_INITILIZATION_ERR, CFE_EVS_EventType_ERROR, 
                      "TELECOM: Pipe initialization error, RC = 0x%08lX", (unsigned long)status);
  }

  // Initialize house keeping telemetry msg
  CFE_MSG_Init(CFE_MSG_PTR(TELECOM_data.hk_tlm.TelemetryHeader), 
               CFE_SB_ValueToMsgId(TELECOM_HK_TLM_MID),
               sizeof(TELECOM_data.hk_tlm));

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
  if (TELECOM_APP_init() != CFE_SUCCESS){
    TELECOM_data.run_status = CFE_ES_RunStatus_APP_ERROR;     
  }

  while (CFE_ES_RunLoop(&TELECOM_data.run_status) == true){
    OS_TaskDelay(TELECOM_PLATFORM_TASK_MSEC);

    TELECOM_process_cmd();
    if (TELECOM_data.downlink_on == true){
      TELECOM_forward_tlm(); 
    }

    TELECOM_forward_ground_cmd();
  }
  CFE_ES_ExitApp(TELECOM_data.run_status);
}

