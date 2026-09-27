#include "cfe_sb.h"
#include "cfe_msg.h"
#include "obc_hk_msg.h"
#include "obc_hk_cmds.h"
#include "obc_hw_lib.h"
#include "obc_hk_eventids.h"
#include "obc_hk_fcncodes.h"
#include "cfe_evs.h"
#include "obc_hk.h"

void OBC_HK_send_hk(const CFE_SB_Buffer_t *sb_buf_p){
  int32 status;

  //    OBC_HK_data.hk_packet.cpu_temp =          OBC_HW_LIB_get_cpu_temp();
  //    OBC_HK_data.hk_packet.ram_usage_percent = OBC_HW_LIB_get_ram_usage_percentage();
  //    OBC_HK_data.hk_packet.ram_usage =         OBC_HW_LIB_get_ram_usage();
  //    OBC_HK_data.hk_packet.cpu_usage =         OBC_HW_LIB_get_cpu_usage();

  OBC_HK_data.hk_tlm.payload.cmd_counter = OBC_HK_data.cmd_counter;
  OBC_HK_data.hk_tlm.payload.err_counter = OBC_HK_data.err_counter;

  status = CFE_SB_TransmitMsg(CFE_MSG_PTR(OBC_HK_data.hk_tlm.TelemetryHeader),
                              true);
  if (status != CFE_SUCCESS)
    CFE_EVS_SendEvent(OBC_HK_MSH_TRANSMITION_ERR_EID, CFE_EVS_EventType_ERROR,
                      "OBC_HK: msg transmition error, RC = 0x%08lX", (unsigned long)status);

}

void OBC_HK_process_ground_cmd(const CFE_SB_Buffer_t *sb_buf_p){
  CFE_MSG_FcnCode_t fcn_code = 0;
  CFE_MSG_GetFcnCode(&sb_buf_p->Msg, &fcn_code);

  switch (fcn_code){
    case OBC_HK_NOOP_CC:
      OBC_HK_noop_cmd((OBC_HK_NoopCmd_t *)sb_buf_p);
      break;
    case OBC_HK_SEND_OBC_INFO_CC:
      break;
  }
}

void OBC_HK_task_pipe(const CFE_SB_Buffer_t *sb_buf_p){ 
  static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
  static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

  CFE_SB_MsgId_t msg_id;

  if (!CFE_SB_IsValidMsgId(CMD_MID)){
    CMD_MID     = CFE_SB_ValueToMsgId(OBC_HK_CMD_MID);
    SEND_HK_MID = CFE_SB_ValueToMsgId(OBC_HK_SEND_HK_MID);
  }

  CFE_MSG_GetMsgId(&sb_buf_p->Msg, &msg_id);

  if (CFE_SB_MsgId_Equal(msg_id, SEND_HK_MID)){
    OBC_HK_send_hk(sb_buf_p); 

  } else if (CFE_SB_MsgId_Equal(msg_id, CMD_MID)){
    OBC_HK_process_ground_cmd(sb_buf_p);

  } else {
    CFE_EVS_SendEvent(OBC_HK_UNDEFINED_MSG_ID, CFE_EVS_EventType_ERROR, "L%d TO: Invalid Msg ID Rcvd 0x%x", __LINE__,
                      (unsigned int)CFE_SB_MsgIdToValue(msg_id));
  }
}
