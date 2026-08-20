#include <stdio.h>
#include <string.h>

#include "common_types.h"
#include "cfe_evs.h"
#include "cfe_es.h"
#include "cfe_sb.h"
#include "cmd_hand.h"

#include "cmd_hand_msgids.h"
#include "cmd_hand_eventids.h"

CMD_HAND_GlobalData_t CMD_HAND_data;


CFE_Status_t CMD_HAND_init(void){
  CFE_Status_t status;

  memset(&CMD_HAND_data, 0, sizeof(CMD_HAND_GlobalData_t));

  CMD_HAND_data.run_status = CFE_ES_RunStatus_APP_RUN;  

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS) {
    CFE_ES_WriteToSysLog("CMD HAND: Error registering for Event Services, RC = 0x%08X, LINE = %d\n",
                         (unsigned int)status,
                         __LINE__);
  }

  status = CFE_SB_CreatePipe(&CMD_HAND_data.cmd_pipe, 
                             CMD_HAND_PLAFORM_PIPE_DEPTH,
                             "CMD_HAND_PIPE");

  if (status == CFE_SUCCESS){
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CMD_HAND_CMD_MID), CMD_HAND_data.cmd_pipe);
    if (status != CFE_SUCCESS){
      CFE_EVS_SendEvent(CMD_HAND_SUBSCRIPTION_CMD_ERR_EID, 
                        CFE_EVS_EventType_ERROR,
                        "Error subscribing to SB Commands, RC = 0x%08X, LINE = %d", 
                        (unsigned int)status,
                        __LINE__);
    }
  } else {
    CFE_EVS_SendEvent(CMD_HAND_SUBSCRIPTION_CMD_ERR_EID, 
                      CFE_EVS_EventType_ERROR,
                      "Error creating SB Commands pipe, RC = 0x%08X, LINE = %d", 
                      (unsigned int)status,
                      __LINE__);

  }

  if (status == CFE_SUCCESS){
    CFE_EVS_SendEvent(CMD_HAND_SUCCESS_EID, 
                      CFE_EVS_EventType_INFORMATION,
                      "CMD HAND intialized successfuly, RC = 0x%08X",
                      (unsigned int)status);
  } else {
    CFE_EVS_SendEvent(CMD_HAND_SUBSCRIPTION_CMD_ERR_EID, 
                      CFE_EVS_EventType_ERROR,
                      "Error initializing the CMD HAND app, RC = 0x%08X, LINE = %d", 
                      (unsigned int)status,
                      __LINE__);  
  }

  return status;
}


void CMD_HAND_AppMain(void){ 
  CFE_Status_t status;

  status = CMD_HAND_init();

  if (status != CFE_SUCCESS){
    CMD_HAND_data.run_status = CFE_ES_RunStatus_APP_ERROR;
  }
  

  while (CFE_ES_RunLoop(&CMD_HAND_data.run_status) == true){ 
    OS_TaskDelay(10000);
  }

  CFE_ES_ExitApp(CMD_HAND_data.run_status);
}
