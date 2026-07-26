#include <stdio.h>

#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"
#include "cfe_msg.h"

#include "obc_hw_lib.h"
#include "obc_hk_msgdefs.h"
#include "obc_hk_msg.h"
#include "obc_hk_eventids.h"
#include "obc_hk.h"

OBC_HK_GlobalApp_t OBC_HK_data;

CFE_Status_t OBC_HK_Init(void){
  int32 status;

  OBC_HK_data.run_status = CFE_ES_RunStatus_APP_RUN;  

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS)
        CFE_ES_WriteToSysLog("OBC_HK: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);

  status = CFE_MSG_Init(CFE_MSG_PTR(OBC_HK_data.hk_packet.telemetry_header),
                        CFE_SB_ValueToMsgId(OBC_HK_TLM_MID),
                        sizeof(OBC_HK_data.hk_packet));

  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(OBC_HK_MSG_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                      "OBC_HK: Error initializing sb meg, RC = 0x%08lX", (unsigned long)status);

    
  if (status == CFE_SUCCESS)
    CFE_EVS_SendEvent(OBC_HK_MSG_INIT_SUCCESFULY_EID, CFE_EVS_EventType_INFORMATION,
                      "OBC_HK: Initialized succesfuly, RC = 0x%08lX", (unsigned long)status);
  

  return status;
}


void OBC_HK_AppMain(void){ 
  int32 status;

  status = OBC_HK_Init();
  if (status != CFE_SUCCESS)
    OBC_HK_data.run_status = CFE_ES_RunStatus_APP_ERROR;

  while (CFE_ES_RunLoop(&OBC_HK_data.run_status) == true){
    OBC_HK_data.hk_packet.cpu_temp = OBC_HW_LIB_get_cpu_temp();
    OBC_HK_data.hk_packet.ram_usage_percent = OBC_HW_LIB_get_ram_usage_percentage();
    OBC_HK_data.hk_packet.ram_usage = OBC_HW_LIB_get_ram_usage();

    status = CFE_SB_TransmitMsg(CFE_MSG_PTR(OBC_HK_data.hk_packet.telemetry_header), true);
    if (status != CFE_SUCCESS)
      CFE_EVS_SendEvent(OBC_HK_MSH_TRANSMITION_ERR_EID, CFE_EVS_EventType_ERROR,
                        "OBC_HK: msg transmition error, RC = 0x%08lX", (unsigned long)status);

    OS_TaskDelay(5000);
  }

  CFE_ES_ExitApp(OBC_HK_data.run_status);
}
