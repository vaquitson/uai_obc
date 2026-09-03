#include "cfe_sb.h"
#include "cfe_msg.h"

void CMD_HAND_task_pipe(const CFE_SB_Buffer_t *SBBufPtr){
  static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
  static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

  CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

  /* cache the local MID Values here, this avoids repeat lookups */
  if (!CFE_SB_IsValidMsgId(CMD_MID))
  {
    CMD_MID     = CFE_SB_ValueToMsgId(SAMPLE_APP_CMD_MID);
    SEND_HK_MID = CFE_SB_ValueToMsgId(SAMPLE_APP_SEND_HK_MID);
  }

  CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

  /* Process all SB messages */
  if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
  {
    /* Housekeeping request */
    SAMPLE_APP_SendHkCmd((const SAMPLE_APP_SendHkCmd_t *)SBBufPtr);
  }
  else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
  {
    /* Ground command */
    SAMPLE_APP_ProcessGroundCommand(SBBufPtr);
  }
  else
{
    /* Unknown command */
    CFE_EVS_SendEvent(SAMPLE_APP_MID_ERR_EID, CFE_EVS_EventType_ERROR, "SAMPLE: invalid command packet,MID = 0x%x",
                      (unsigned int)CFE_SB_MsgIdToValue(MsgId));
  }

}
