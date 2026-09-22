#include "cfe_evs.h"
#include "cfe_msg.h"
#include "telecom_dispatch.h"
#include "telecom_msgids.h"
#include "telecom_eventids.h"


void TELECOM_task_pipe(const CFE_SB_Buffer_t *sb_buf_p){
  static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
  static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

  if (!CFE_SB_IsValidMsgId(CMD_MID)){
    CMD_MID     = CFE_SB_ValueToMsgId(TELECOM_CMD_MID);
    SEND_HK_MID = CFE_SB_ValueToMsgId(TELECOM_SEND_HK_MID);
  }

  CFE_SB_MsgId_t msg_id;

  CFE_MSG_GetMsgId(&sb_buf_p->Msg, &msg_id);

  if (CFE_SB_MsgId_Equal(msg_id, SEND_HK_MID)){
  } else if (CFE_SB_MsgId_Equal(msg_id, CMD_MID)){

  } else {
    // unrecognized event id
    CFE_EVS_SendEvent(TELECOM_MID_ERR_EID, CFE_EVS_EventType_ERROR, "L%d TO: Invalid Msg ID Rcvd 0x%x", __LINE__,
                      (unsigned int)CFE_SB_MsgIdToValue(msg_id));
  }
}
