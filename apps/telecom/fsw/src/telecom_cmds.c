#include "telecom_cmds.h"
#include "telecom_msg.h"
#include "telecom_app.h"
#include "common_types.h"

CFE_Status_t TELECOM_noop_cmd(const TELECOM_NoopCmd_t *data_p){ 
  return CFE_SUCCESS;
}

CFE_Status_t TELECOM_send_hk_cmd(const TELECOM_SendHkCmd_t *data){
  TELECOM_data.hk_tlm.payload.command_counter = TELECOM_data.cmd_counter;   
  TELECOM_data.hk_tlm.payload.err_counter     = TELECOM_data.err_counter;   

  CFE_SB_TimeStampMsg(CFE_MSG_PTR(TELECOM_data.hk_tlm.TelemetryHeader));
  CFE_SB_TransmitMsg(CFE_MSG_PTR(TELECOM_data.hk_tlm.TelemetryHeader), true);

  return CFE_SUCCESS;
}
