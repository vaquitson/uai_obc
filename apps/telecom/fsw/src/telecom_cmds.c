#include "telecom_cmds.h"
#include "telecom_eventids.h"
#include "telecom_msg.h"
#include "telecom_app.h"
#include "common_types.h"
#include "cfe_evs.h"
#include "telecom_tlm.h"

CFE_Status_t TELECOM_noop_cmd(const TELECOM_NoopCmd_t *data_p){ 
  CFE_EVS_SendEvent(TELECOM_CMD_RECIVED, 
                    CFE_EVS_EventType_INFORMATION,
                    "TELECOM: Noop Recived");
  return CFE_SUCCESS;
}

CFE_Status_t TELECOM_send_hk_cmd(const TELECOM_SendHkCmd_t *data){
  CFE_EVS_SendEvent(TELECOM_CMD_RECIVED, CFE_EVS_EventType_INFORMATION,
                    "TELECOM: TELECOM_send_hk_cmd recived");

  TELECOM_data.hk_tlm.payload.command_counter = TELECOM_data.cmd_counter;   
  TELECOM_data.hk_tlm.payload.err_counter     = TELECOM_data.err_counter;   

  CFE_SB_TimeStampMsg(CFE_MSG_PTR(TELECOM_data.hk_tlm.TelemetryHeader));
  CFE_SB_TransmitMsg(CFE_MSG_PTR(TELECOM_data.hk_tlm.TelemetryHeader), true);
  return CFE_SUCCESS;
}

CFE_Status_t TELECOM_open_tlm_cmd(const TELECOM_OpenTlmCmd_t *data){
  CFE_Status_t status = 0;
  CFE_EVS_SendEvent(TELECOM_CMD_RECIVED, 
                    CFE_EVS_EventType_INFORMATION,
                    "TELECOM: TELECOM_open_tlm_cmd recived");

  // HW dependent
  status = TELECOM_open_tlm(&(data->payload));

  printf("status = %d\n", status);
  TELECOM_OpenTlmTlm_t msg;    
  CFE_MSG_Init(CFE_MSG_PTR(msg.TelemetryHeader),CFE_SB_ValueToMsgId(TELECOM_OPEN_TLM_MID), sizeof(msg));
  msg.payload.status_code = status;

  CFE_SB_TimeStampMsg(CFE_MSG_PTR(msg.TelemetryHeader));
  CFE_SB_TransmitMsg(CFE_MSG_PTR(msg.TelemetryHeader), true);

  return status;
}

