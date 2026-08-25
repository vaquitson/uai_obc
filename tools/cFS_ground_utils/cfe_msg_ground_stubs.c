#include "cfe_platform_cfg.h"
#include "cfe_sb.h"

/*
 * Ground utilities can use the message library without running the full
 * Software Bus. CFE_MSG_SetMsgId() only needs this validity check.
 */
bool CFE_SB_IsValidMsgId(CFE_SB_MsgId_t MsgId)
{
    return CFE_SB_MsgIdToValue(MsgId) <= CFE_PLATFORM_SB_HIGHEST_VALID_MSGID;
}
