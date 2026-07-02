/**
 * @file
 *   OBC_HK Application Topic IDs
 */
#ifndef DEFAULT_OBC_HK_MSGID_VALUES_H
#define DEFAULT_OBC_HK_MSGID_VALUES_H

#include "cfe_core_api_base_msgids.h"

// [WARNING: Absloute sheet]
#include "../fsw/inc/obc_hk_topicids.h"


#define OBC_HK_CMD_PLATFORM_MIDVAL(x) CFE_PLATFORM_CMD_TOPICID_TO_MIDV(OBC_HK_MISSION_##x##_TOPICID)
#define OBC_HK_TLM_PLATFORM_MIDVAL(x) CFE_PLATFORM_TLM_TOPICID_TO_MIDV(OBC_HK_MISSION_##x##_TOPICID)

#endif
