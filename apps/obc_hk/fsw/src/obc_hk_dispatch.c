#include "cfe_sb.h"
#include "cfe_msg.h"
#include "obc_hk_msg.h"
#include "obc_hk_cmds.h"
#include "obc_hw_lib.h"
#include "obc_hk_eventids.h"
#include "cfe_evs.h"
#include "obc_hk.h"

void OBC_HK_send_hk(void){
    int32 status;

    OBC_HK_data.hk_packet.cpu_temp =          OBC_HW_LIB_get_cpu_temp();
    OBC_HK_data.hk_packet.ram_usage_percent = OBC_HW_LIB_get_ram_usage_percentage();
    OBC_HK_data.hk_packet.ram_usage =         OBC_HW_LIB_get_ram_usage();
    OBC_HK_data.hk_packet.cpu_usage =         OBC_HW_LIB_get_cpu_usage();

    status = CFE_SB_TransmitMsg(CFE_MSG_PTR(OBC_HK_data.hk_packet.telemetry_header),
                                true);
    if (status != CFE_SUCCESS)
      CFE_EVS_SendEvent(OBC_HK_MSH_TRANSMITION_ERR_EID, CFE_EVS_EventType_ERROR,
                        "OBC_HK: msg transmition error, RC = 0x%08lX", (unsigned long)status);

}

void OBC_HK_task_pipe(const CFE_SB_Buffer_t *sb_buf_p){ 
  CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

  CFE_MSG_GetMsgId(&sb_buf_p->Msg, &MsgId);

  if (CFE_SB_MsgId_Equal(MsgId, CFE_SB_ValueToMsgId(OBC_HK_SEND_TLM_MID))){
    OBC_HK_send_hk(); 
  }
}
