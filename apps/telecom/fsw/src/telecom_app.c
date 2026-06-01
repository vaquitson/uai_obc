#include "telecom_app.h"
#include "cfe_evs.h"
#include "cfe_es.h"
#include "telecom_eventids.h"


void TELECOM_AppMain(void){
  TELECOM_APP_Init();
}


CFE_Status_t TELECOM_APP_Init(void){ 
  CFE_Status_t status;

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS){
    CFE_ES_WriteToSysLog("TELECOM: Error registering for Event Services, RC = 0x%08X\n", (unsigned int)status);
  }

  
  CFE_EVS_SendEvent(TELECOM_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "Telecom Initialized");

  return CFE_SUCCESS;
}
