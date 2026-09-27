#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"
#include "cfe_msg.h"
#include "osapi.h"

#include "payload_app_msgdefs.h"
#include "payload_app_msg.h"
#include "payload_app_eventids.h"
#include "payload_app_fcncodes.h"
#include "payload_app_socket.h"
#include "payload_app_data.h"
#include "payload_app.h"

PAYLOAD_APP_GlobalApp_t PAYLOAD_APP_Global;

CFE_Status_t PAYLOAD_APP_Init(void)
{
  int32 status;

  memset(&PAYLOAD_APP_Global, 0, sizeof(PAYLOAD_APP_Global));

  PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_RUN;

  status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
  if (status != CFE_SUCCESS)
    CFE_ES_WriteToSysLog("PAYLOAD_APP: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);

  status = CFE_MSG_Init(CFE_MSG_PTR(PAYLOAD_APP_Global.hk_packet.telemetry_header),
                         CFE_SB_ValueToMsgId(PAYLOAD_APP_TLM_MID), sizeof(PAYLOAD_APP_Global.hk_packet));
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error initializing HK msg, RC = 0x%08lX", (unsigned long)status);

  status = CFE_MSG_Init(CFE_MSG_PTR(PAYLOAD_APP_Global.data_msg.telemetry_header),
                         CFE_SB_ValueToMsgId(PAYLOAD_APP_DATA_TLM_MID), sizeof(PAYLOAD_APP_Global.data_msg));
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error initializing data msg, RC = 0x%08lX", (unsigned long)status);

  status = CFE_SB_CreatePipe(&PAYLOAD_APP_Global.cmd_pipe, PAYLOAD_APP_PIPE_DEPTH, PAYLOAD_APP_PIPE_NAME);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: Error creating cmd pipe, RC = 0x%08lX", (unsigned long)status);

  if (status == CFE_SUCCESS)
  {
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(PAYLOAD_APP_CMD_MID), PAYLOAD_APP_Global.cmd_pipe);
    if (status != CFE_SUCCESS)
      CFE_EVS_SendEvent(PAYLOAD_APP_INIT_FAILURE_EID, CFE_EVS_EventType_ERROR,
                         "PAYLOAD_APP: Error subscribing to commands, RC = 0x%08lX", (unsigned long)status);
  }

  PAYLOAD_APP_SocketDirEnsure();

  if (status == CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_INIT_SUCCESSFUL_EID, CFE_EVS_EventType_INFORMATION,
                       "PAYLOAD_APP: Initialized successfully, RC = 0x%08lX", (unsigned long)status);

  return status;
}

void PAYLOAD_APP_AppMain(void)
{
  int32 status;
  CFE_SB_Buffer_t *sb_buf_ptr;

  status = PAYLOAD_APP_Init();
  if (status != CFE_SUCCESS)
    PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_ERROR;

  while (CFE_ES_RunLoop(&PAYLOAD_APP_Global.run_status) == true)
  {
    status = CFE_SB_ReceiveBuffer(&sb_buf_ptr, PAYLOAD_APP_Global.cmd_pipe, PAYLOAD_APP_SB_TIMEOUT_MS);

    if (status == CFE_SUCCESS)
      PAYLOAD_APP_ProcessCommandPacket(sb_buf_ptr);
    else if (status != CFE_SB_TIME_OUT)
    {
      CFE_ES_WriteToSysLog("PAYLOAD_APP: SB pipe read error, RC = 0x%08lX\n", (unsigned long)status);
      PAYLOAD_APP_Global.run_status = CFE_ES_RunStatus_APP_ERROR;
    }

    PAYLOAD_APP_SendHk();

    if (!PAYLOAD_APP_SocketIsConnected())
      PAYLOAD_APP_SocketConnect();

    if (PAYLOAD_APP_SocketIsConnected())
    {
      size_t json_len = 0;
      int32  frame_status = PAYLOAD_APP_SocketReadFrame(PAYLOAD_APP_Global.sock_buf,
                                                          sizeof(PAYLOAD_APP_Global.sock_buf), &json_len);

      if (frame_status == PAYLOAD_APP_SOCK_FRAME_OK)
        PAYLOAD_APP_ParseAndPublish(PAYLOAD_APP_Global.sock_buf, json_len);
    }

    PAYLOAD_APP_Global.hk_packet.socket_connected = PAYLOAD_APP_SocketIsConnected();
  }

  PAYLOAD_APP_SocketClose();
  CFE_ES_ExitApp(PAYLOAD_APP_Global.run_status);
}

void PAYLOAD_APP_ProcessCommandPacket(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  CFE_SB_MsgId_t msg_id = CFE_SB_INVALID_MSG_ID;

  CFE_MSG_GetMsgId(&sb_buf_ptr->Msg, &msg_id);

  switch (CFE_SB_MsgIdToValue(msg_id))
  {
    case PAYLOAD_APP_CMD_MID:
      PAYLOAD_APP_ProcessGroundCommand(sb_buf_ptr);
      break;

    default:
      CFE_EVS_SendEvent(PAYLOAD_APP_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                         "PAYLOAD_APP: invalid command pipe message ID: 0x%X",
                         (unsigned int)CFE_SB_MsgIdToValue(msg_id));
      break;
  }
}

void PAYLOAD_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *sb_buf_ptr)
{
  CFE_MSG_FcnCode_t fcn_code = 0;

  CFE_MSG_GetFcnCode(&sb_buf_ptr->Msg, &fcn_code);

  switch (fcn_code)
  {
    case PAYLOAD_APP_NOOP_CC:
      PAYLOAD_APP_Noop();
      break;

    case PAYLOAD_APP_RESET_COUNTERS_CC:
      PAYLOAD_APP_ResetCounters();
      break;

    default:
      PAYLOAD_APP_Global.hk_packet.err_counter++;
      CFE_EVS_SendEvent(PAYLOAD_APP_INVALID_CC_EID, CFE_EVS_EventType_ERROR,
                         "PAYLOAD_APP: invalid command code: %u", (unsigned int)fcn_code);
      break;
  }
}

void PAYLOAD_APP_Noop(void)
{
  PAYLOAD_APP_Global.hk_packet.cmd_counter++;
  CFE_EVS_SendEvent(PAYLOAD_APP_NOOP_EID, CFE_EVS_EventType_INFORMATION, "PAYLOAD_APP: NOOP command received");
}

void PAYLOAD_APP_ResetCounters(void)
{
  PAYLOAD_APP_Global.hk_packet.cmd_counter = 0;
  PAYLOAD_APP_Global.hk_packet.err_counter = 0;
  CFE_EVS_SendEvent(PAYLOAD_APP_RESET_COUNTERS_EID, CFE_EVS_EventType_INFORMATION,
                     "PAYLOAD_APP: counters reset");
}

void PAYLOAD_APP_SendHk(void)
{
  int32 status;

  status = CFE_SB_TransmitMsg(CFE_MSG_PTR(PAYLOAD_APP_Global.hk_packet.telemetry_header), true);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(PAYLOAD_APP_HK_TRANSMIT_ERR_EID, CFE_EVS_EventType_ERROR,
                       "PAYLOAD_APP: HK transmit error, RC = 0x%08lX", (unsigned long)status);
}
