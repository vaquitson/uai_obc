/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

#include "sample_app.h"
#include "sample_app_cmds.h"
#include "sample_app_utils.h"
#include "sample_app_eventids.h"
#include "sample_app_dispatch.h"
#include "sample_app_tbl.h"
#include "sample_app_version.h"

#define NARWAL_TLM_MID 987

SAMPLE_APP_Data_t SAMPLE_APP_Data;


// Narwal
typedef struct
{
  CFE_MSG_TelemetryHeader_t TelemetryHeader;
  char mesage;

} NARWAL_paquet_t;


void SAMPLE_APP_Main(void)
{
  CFE_Status_t     status;
  CFE_SB_Buffer_t *SBBufPtr;

  status = SAMPLE_APP_Init();

  if (status != CFE_SUCCESS){
    SAMPLE_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
  }

  // Narwal
  NARWAL_paquet_t narwal_paquet; 
  CFE_SB_MsgId_t narwal_paquet_mid = CFE_SB_ValueToMsgId(NARWAL_TLM_MID);


  // Narwal
  CFE_MSG_Init(CFE_MSG_PTR(narwal_paquet.TelemetryHeader),
               narwal_paquet_mid,
               sizeof(narwal_paquet));

  narwal_paquet.mesage = 'a';

  while (CFE_ES_RunLoop(&SAMPLE_APP_Data.RunStatus) == true) {
    CFE_ES_PerfLogExit(SAMPLE_APP_PERF_ID);

    status = CFE_SB_ReceiveBuffer(&SBBufPtr, SAMPLE_APP_Data.CommandPipe, 5000);

    if (status == CFE_SUCCESS) {
      SAMPLE_APP_TaskPipe(SBBufPtr);
    }


    // Narwal
    status = CFE_SB_TransmitMsg(CFE_MSG_PTR(narwal_paquet.TelemetryHeader), true);

    if (status == CFE_SUCCESS){
      //printf("Narwal Message send from sample app\n");
    }

  }

    CFE_ES_ExitApp(SAMPLE_APP_Data.RunStatus);
  }


CFE_Status_t SAMPLE_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[SAMPLE_APP_CFG_MAX_VERSION_STR_LEN];

    memset(&SAMPLE_APP_Data, 0, sizeof(SAMPLE_APP_Data));

    SAMPLE_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS) {
        CFE_ES_WriteToSysLog("Sample App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else { 
        CFE_MSG_Init(CFE_MSG_PTR(SAMPLE_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(SAMPLE_APP_HK_TLM_MID),
                     sizeof(SAMPLE_APP_Data.HkTlm));

        status = CFE_SB_CreatePipe(&SAMPLE_APP_Data.CommandPipe, SAMPLE_APP_PLATFORM_PIPE_DEPTH,
                                   SAMPLE_APP_PLATFORM_PIPE_NAME);

        if (status != CFE_SUCCESS){
            CFE_EVS_SendEvent(SAMPLE_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Sample App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS){
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SAMPLE_APP_SEND_HK_MID), SAMPLE_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SAMPLE_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Sample App: Error Subscribing to HK request, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ground command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SAMPLE_APP_CMD_MID), SAMPLE_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SAMPLE_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Sample App: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Register Example Table(s)
        */
        status = CFE_TBL_Register(&SAMPLE_APP_Data.TblHandles[0], "ExampleTable", sizeof(SAMPLE_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, SAMPLE_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SAMPLE_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Sample App: Error Registering Example Table, RC = 0x%08lX", (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(SAMPLE_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, SAMPLE_APP_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, SAMPLE_APP_CFG_MAX_VERSION_STR_LEN, "Sample App", SAMPLE_APP_VERSION,
                                    SAMPLE_APP_BUILD_CODENAME, SAMPLE_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(SAMPLE_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "Sample App Initialized.%s",
                          VersionString);
    }

    return status;
}
