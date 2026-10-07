#include <stdio.h>

#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"
#include "cfe_msg.h"

#include "obc_hk.h"
#include "obc_hw_lib.h"
#include "obc_hk_msgdefs.h"
#include "obc_hk_msg.h"
#include "obc_hk_eventids.h"
#include "obc_hk_dispatch.h"
#include "obc_hk_internal_cfg.h"

// temporal for quick testing
#include "telecom_msg.h"

OBC_HK_GlobalApp_t OBC_HK_data = {0};

CFE_Status_t OBC_HK_pipe_init(void){
  CFE_Status_t status;

  strncpy(OBC_HK_data.cmd_pip_name, OBC_HK_CMD_PIPE_NAME, OBC_HK_PIPE_NAME_MAX);
  status = CFE_SB_CreatePipe(&OBC_HK_data.cmd_pipe, 
                             OBC_HK_PLAFORM_PIPE_DEPTH,
                             OBC_HK_CMD_PIPE_NAME); 

  CFE_SB_Subscribe(CFE_SB_ValueToMsgId(OBC_HK_CMD_MID), OBC_HK_data.cmd_pipe);

  if (status != CFE_SUCCESS){
   CFE_EVS_SendEvent(OBC_HK_PIPE_CREATION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "OBC_HK: Error initializing cmd pipe, RC = 0x%08lX", (unsigned long)status);  
  }

  return status;
}


CFE_Status_t OBC_HK_Init(void){
  CFE_Status_t status;

  OBC_HK_data.run_status = CFE_ES_RunStatus_APP_RUN;  

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS){
    CFE_ES_WriteToSysLog("OBC_HK: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    return status;
  }

  status = OBC_HK_pipe_init();
  if (status == CFE_SUCCESS){
    status = CFE_MSG_Init(CFE_MSG_PTR(OBC_HK_data.hk_tlm.TelemetryHeader),
                          CFE_SB_ValueToMsgId(OBC_HK_HK_MID),
                          sizeof(OBC_HK_data.hk_tlm));

    if (status != CFE_SUCCESS){
      CFE_EVS_SendEvent(OBC_HK_MSG_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                        "OBC_HK: Error initializing sb meg, RC = 0x%08lX", (unsigned long)status);
    }
  }

  if (status == CFE_SUCCESS){
    CFE_EVS_SendEvent(OBC_HK_MSG_INIT_SUCCESFULY_EID, CFE_EVS_EventType_INFORMATION,
                      "OBC_HK: Initialized succesfuly, RC = 0x%08lX", (unsigned long)status); 
  }

  return status;
}


void OBC_HK_AppMain(void){ 
  CFE_Status_t status;
  CFE_SB_Buffer_t *sb_buf_p;

  // temporal for quick testing
  bool vooolatile = false;

  status = OBC_HK_Init();
  if (status != CFE_SUCCESS)
    OBC_HK_data.run_status = CFE_ES_RunStatus_APP_ERROR;

  while (CFE_ES_RunLoop(&OBC_HK_data.run_status) == true){

    // temporal for quick testing
    if (vooolatile == false){
      TELECOM_OpenTlmCmd_t msg_open_tlm; 
      TELECOM_SendHkCmd_t msg_hk; 
      CFE_MSG_Init(CFE_MSG_PTR(msg_open_tlm.CommandHeader),
                   CFE_SB_ValueToMsgId(0x187A),
                   sizeof(msg_open_tlm));

      CFE_MSG_Init(CFE_MSG_PTR(msg_hk.CommandHeader),
                   CFE_SB_ValueToMsgId(123),
                   sizeof(msg_hk));

      CFE_MSG_SetFcnCode(CFE_MSG_PTR(msg_open_tlm.CommandHeader), 2);
      strcpy(msg_open_tlm.payload.dest_IP, "127.0.0.1");
      strcpy(msg_open_tlm.payload.dest_port, "2234");

      OS_TaskDelay(2000);
      CFE_SB_TransmitMsg(CFE_MSG_PTR(msg_open_tlm.CommandHeader), true);

      OS_TaskDelay(1000);
      CFE_SB_TransmitMsg(CFE_MSG_PTR(msg_hk.CommandHeader), true);
      vooolatile = true;
    }

    status = CFE_SB_ReceiveBuffer(&sb_buf_p, 
                                  OBC_HK_data.cmd_pipe, 
                                  CFE_SB_PEND_FOREVER);
    
    if (status == CFE_SUCCESS){
      printf("FROM OBC_HK\n");
      OBC_HK_task_pipe(sb_buf_p);
    }
  }
  CFE_ES_ExitApp(OBC_HK_data.run_status);
}
